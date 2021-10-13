// This file is part of KWIVER, and is distributed under the
// OSI-approved BSD 3-Clause License. See top-level LICENSE file or
// https://github.com/Kitware/kwiver/blob/master/LICENSE for details.

/// \file
/// Interface to the KLV data formats.

#ifndef KWIVER_ARROWS_KLV_KLV_DATA_FORMAT_H_
#define KWIVER_ARROWS_KLV_KLV_DATA_FORMAT_H_

#include "klv_blob.txx"
#include "klv_key.h"
#include "klv_read_write.txx"
#include "klv_value.h"

#include <arrows/klv/kwiver_algo_klv_export.h>
#include <vital/exceptions/metadata.h>
#include <vital/logger/logger.h>

#include <memory>
#include <ostream>
#include <sstream>
#include <typeinfo>
#include <vector>

namespace kwiver {

namespace arrows {

namespace klv {

// ----------------------------------------------------------------------------
using klv_read_iter_t = typename klv_bytes_t::const_iterator;
using klv_write_iter_t = typename klv_bytes_t::iterator;

// ----------------------------------------------------------------------------
/// Untyped base for KLV data formats.
///
/// This class provides an interface to the KLV data formats, providing read,
/// write, and printing capabilities.
class KWIVER_ALGO_KLV_EXPORT klv_data_format
{
public:
  /// \param fixed_length The exact length in bytes of this data format. If
  /// zero, the length is variable.
  explicit
  klv_data_format( size_t fixed_length );

  virtual
  ~klv_data_format() = default;

  /// Parse raw bytes into data type; return as \c klv_value.
  virtual klv_value
  read( klv_read_iter_t& data, size_t length ) const = 0;

  /// Write \c klv_value (holding proper type) to raw bytes.
  virtual void
  write( klv_value const& value, klv_write_iter_t& data,
         size_t length ) const = 0;

  /// Return number of bytes required to write \p value.
  virtual size_t
  length_of( klv_value const& value ) const = 0;

  /// Return \c type_info for read / written type.
  virtual std::type_info const&
  type() const = 0;

  /// Return name of read / written type.
  std::string
  type_name() const;

  /// Print a string representation of \p value to \p os.
  virtual std::ostream&
  print( std::ostream& os, klv_value const& value ) const = 0;

  /// Return a string representation of \p value.
  std::string
  to_string( klv_value const& value ) const;

  /// Return a textual description of this data format.
  virtual std::string
  description() const = 0;

protected:
  /// Describe the length of this data format.
  std::string
  length_description() const;

  size_t m_fixed_length;
};

using klv_data_format_sptr = std::shared_ptr< klv_data_format >;

// ----------------------------------------------------------------------------
/// Typed base for KLV data formats.
///
/// This class implements the functionality common to data formats of a
/// particular type. It takes care of checking for common edge cases like being
/// given empty data or invalid lengths, so specific derived data formats
/// don't need to duplicate that boilerplate in each class. Specific formats
/// only have to worry about overriding the \c *_typed() functions.
template < class T >
class KWIVER_ALGO_KLV_EXPORT klv_data_format_ : public klv_data_format
{
public:
  using data_type = T;

  explicit
  klv_data_format_( size_t fixed_length ) : klv_data_format{ fixed_length }
  {}

  virtual
  ~klv_data_format_() = default;

  klv_value
  read( klv_read_iter_t& data, size_t length ) const override final
  {
    if( !length )
    {
      // Zero length: null / unknown value
      return klv_value{};
    }
    else if( m_fixed_length && length != m_fixed_length )
    {
      // Invalid length
      std::stringstream ss;
      ss        << "fixed-length format `" << description()
                << "` received wrong number of bytes ( " << length << " )";
      VITAL_THROW( kwiver::vital::metadata_exception, ss.str() );
    }

    try
    {
      // Try to parse using this data format
      return klv_value{ read_typed( data, length ), length };
    }
    catch ( std::exception const& e )
    {
      // Return blob if parsing failed
      LOG_ERROR( kwiver::vital::get_logger( "klv.read" ),
                 "error occurred during parsing: " << e.what() );
      return klv_value{ klv_read_blob( data, length ), length };
    }
  }

  void
  write( klv_value const& value, klv_write_iter_t& data,
         size_t max_length ) const override final
  {
    if( value.empty() )
    {
      // Null / unknown value: write nothing
      return;
    }
    else if( !value.valid() )
    {
      // Unparsed value: write raw bytes
      klv_write_blob( value.get< klv_blob >(), data, max_length );
    }
    else
    {
      // Ensure we have enough bytes
      auto const value_length = length_of( value );
      if( value_length > max_length )
      {
        VITAL_THROW( kwiver::vital::metadata_buffer_overflow,
                     "write will overflow buffer" );
      }

      // Write the value
      auto const begin = data;
      write_typed( value.get< T >(), data, value_length );

      // Ensure the number of bytes we wrote was how many we said we were going
      // to write
      auto const written_length =
        static_cast< size_t >( std::distance( begin, data ) );
      if( written_length != value_length )
      {
        std::stringstream ss;
        ss      << "format `" << description() << "`: "
                << "written length (" << written_length << ") and "
                << "calculated length (" << value_length <<  ") not equal";
        throw std::logic_error( ss.str() );
      }
    }
  }

  size_t
  length_of( klv_value const& value ) const override final
  {
    if( value.empty() )
    {
      return 0;
    }
    else if( !value.valid() )
    {
      return value.get< klv_blob >()->size();
    }
    else
    {
      return m_fixed_length
             ? m_fixed_length
             : length_of_typed( value.get< T >(), value.length_hint() );
    }
  }

  std::type_info const&
  type() const override final
  {
    return typeid( T );
  }

  std::ostream&
  print( std::ostream& os, klv_value const& value ) const override final
  {
    return !value.valid()
           ? ( os << value )
           : print_typed( os, value.get< T >(), value.length_hint() );
  }

protected:
  // These functions are overridden by the specific data format classes.
  // length_of_typed() and print_typed() have default behavior provided to
  // make overriding optional.

  virtual T
  read_typed( klv_read_iter_t& data, size_t length ) const = 0;

  virtual void
  write_typed( T const& value, klv_write_iter_t& data,
               size_t length ) const = 0;

  virtual size_t
  length_of_typed( VITAL_UNUSED T const& value, size_t length_hint ) const
  {
    if( length_hint )
    {
      return length_hint;
    }

    std::stringstream ss;
    ss  << "application must provide length of variable-length format `"
        << description() << "`";
    throw std::logic_error( ss.str() );
  }

  virtual std::ostream&
  print_typed( std::ostream& os, T const& value,
               VITAL_UNUSED size_t length_hint ) const
  {
    if( std::is_same< T, std::string >::value )
    {
      return os << '"' << value << '"';
    }
    else
    {
      return os << value;
    }
  }
};

// ----------------------------------------------------------------------------
/// Treats data as a binary blob, or uninterpreted sequence of bytes.
class KWIVER_ALGO_KLV_EXPORT klv_blob_format
  : public klv_data_format_< klv_blob >
{
public:
  klv_blob_format( size_t fixed_length = 0 );

  virtual
  ~klv_blob_format() = default;

  std::string
  description() const override;

protected:
  klv_blob
  read_typed( klv_read_iter_t& data, size_t length ) const override;

  void
  write_typed( klv_blob const& value,
               klv_write_iter_t& data, size_t length ) const override;

  size_t
  length_of_typed( klv_blob const& value, size_t length_hint ) const override;
};

// ----------------------------------------------------------------------------
/// Interprets data as a string.
class KWIVER_ALGO_KLV_EXPORT klv_string_format
  : public klv_data_format_< std::string >
{
public:
  klv_string_format( size_t fixed_length = 0 );

  std::string
  description() const override;

protected:
  std::string
  read_typed( klv_read_iter_t& data, size_t length ) const override;

  void
  write_typed( std::string const& value,
               klv_write_iter_t& data, size_t length ) const override;

  size_t
  length_of_typed( std::string const& value,
                   size_t length_hint ) const override;
};

// ----------------------------------------------------------------------------
/// Interprets data as an unsigned integer.
class KWIVER_ALGO_KLV_EXPORT klv_uint_format
  : public klv_data_format_< uint64_t >
{
public:
  klv_uint_format( size_t fixed_length = 0 );

  virtual
  ~klv_uint_format() = default;

  std::string
  description() const override;

protected:
  uint64_t
  read_typed( klv_read_iter_t& data, size_t length ) const override;

  void
  write_typed( uint64_t const& value,
               klv_write_iter_t& data, size_t length ) const override;

  size_t
  length_of_typed( uint64_t const& value, size_t length_hint ) const override;
};

// ----------------------------------------------------------------------------
/// Interprets data as a signed integer.
class KWIVER_ALGO_KLV_EXPORT klv_sint_format
  : public klv_data_format_< int64_t >
{
public:
  klv_sint_format( size_t fixed_length = 0 );

  virtual
  ~klv_sint_format() = default;

  std::string
  description() const override;

protected:
  int64_t
  read_typed( klv_read_iter_t& data, size_t length ) const override;

  void
  write_typed( int64_t const& value,
               klv_write_iter_t& data, size_t length ) const override;

  size_t
  length_of_typed( int64_t const& value, size_t length_hint ) const override;
};

// ----------------------------------------------------------------------------
/// Interprets data as an enum type.
template < class T >
class KWIVER_ALGO_KLV_EXPORT klv_enum_format : public klv_data_format_< T >
{
public:
  klv_enum_format( size_t fixed_length = 1 )
    : klv_data_format_< T >{ fixed_length }
  {}

  virtual
  ~klv_enum_format() = default;

  std::string
  description() const override
  {
    std::stringstream ss;
    ss << this->type_name() << " enumeration of "
       << this->length_description();
    return ss.str();
  }

protected:
  T
  read_typed( klv_read_iter_t& data, size_t length ) const override
  {
    return static_cast< T >( klv_read_int< uint64_t >( data, length ) );
  }

  void
  write_typed( T const& value,
               klv_write_iter_t& data, size_t length ) const override
  {
    klv_write_int( static_cast< uint64_t >( value ), data, length );
  }

  size_t
  length_of_typed( T const& value,
                   VITAL_UNUSED size_t length_hint ) const override
  {
    return klv_int_length( static_cast< uint64_t >( value ) );
  }

  size_t m_length;
};

// ----------------------------------------------------------------------------
/// Interprets data as an unsigned integer encoded in BER format.
class KWIVER_ALGO_KLV_EXPORT klv_ber_format
  : public klv_data_format_< uint64_t >
{
public:
  klv_ber_format();

  virtual
  ~klv_ber_format() = default;

  std::string
  description() const override;

protected:
  uint64_t
  read_typed( klv_read_iter_t& data, size_t length ) const override;

  void
  write_typed( uint64_t const& value,
               klv_write_iter_t& data, size_t length ) const override;

  size_t
  length_of_typed( uint64_t const& value, size_t length_hint ) const override;
};

// ----------------------------------------------------------------------------
/// Interprets data as an unsigned integer encoded in BER-OID format.
class KWIVER_ALGO_KLV_EXPORT klv_ber_oid_format
  : public klv_data_format_< uint64_t >
{
public:
  klv_ber_oid_format();

  virtual
  ~klv_ber_oid_format() = default;

  std::string
  description() const override;

protected:
  uint64_t
  read_typed( klv_read_iter_t& data, size_t length ) const override;

  void
  write_typed( uint64_t const& value,
               klv_write_iter_t& data, size_t length ) const override;

  size_t
  length_of_typed( uint64_t const& value, size_t length_hint ) const override;
};

// ----------------------------------------------------------------------------
/// Interprets data as IEEE-754 floating point value.
class KWIVER_ALGO_KLV_EXPORT klv_float_format
  : public klv_data_format_< double >
{
public:
  klv_float_format( size_t fixed_length = 0 );

  virtual
  ~klv_float_format() = default;

  std::string
  description() const override;

protected:
  double
  read_typed( klv_read_iter_t& data, size_t length ) const override;

  void
  write_typed( double const& value,
               klv_write_iter_t& data, size_t length ) const override;

  std::ostream&
  print_typed( std::ostream& os, double const& value,
               size_t length_hint ) const override;
};

// ----------------------------------------------------------------------------
/// Interprets data as an unsigned integer mapped to a known floating-point
/// range.
class KWIVER_ALGO_KLV_EXPORT klv_sflint_format
  : public klv_data_format_< double >
{
public:
  klv_sflint_format( double minimum, double maximum, size_t fixed_length = 0 );

  virtual
  ~klv_sflint_format() = default;

  std::string
  description() const override;

protected:
  double
  read_typed( klv_read_iter_t& data, size_t length ) const override;

  void
  write_typed( double const& value,
               klv_write_iter_t& data, size_t length ) const override;

  std::ostream&
  print_typed( std::ostream& os, double const& value,
               size_t length_hint ) const override;

  double m_minimum;
  double m_maximum;
};

// ----------------------------------------------------------------------------
/// Interprets data as an signed integer mapped to a known floating-point
/// range.
class KWIVER_ALGO_KLV_EXPORT klv_uflint_format
  : public klv_data_format_< double >
{
public:
  klv_uflint_format( double minimum, double maximum, size_t fixed_length = 0 );

  virtual
  ~klv_uflint_format() = default;

  std::string
  description() const override;

protected:
  double
  read_typed( klv_read_iter_t& data, size_t length ) const override;

  void
  write_typed( double const& value,
               klv_write_iter_t& data, size_t length ) const override;

  std::ostream&
  print_typed( std::ostream& os, double const& value,
               size_t length_hint ) const override;

  double m_minimum;
  double m_maximum;
};

// ----------------------------------------------------------------------------
/// Interprets data as a floating point value encoded in IMAP format.
class KWIVER_ALGO_KLV_EXPORT klv_imap_format
  : public klv_data_format_< double >
{
public:
  klv_imap_format( double minimum, double maximum );

  std::string
  description() const override;

protected:
  double
  read_typed( klv_read_iter_t& data, size_t length ) const override;

  void
  write_typed( double const& value,
               klv_write_iter_t& data, size_t length ) const override;

  std::ostream&
  print_typed( std::ostream& os, double const& value,
               size_t length_hint ) const override;

  double m_minimum;
  double m_maximum;
};

} // namespace klv

} // namespace arrows

} // namespace kwiver

#endif
