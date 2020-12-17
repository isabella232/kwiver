// This file is part of KWIVER, and is distributed under the
// OSI-approved BSD 3-Clause License. See top-level LICENSE file or
// https://github.com/Kitware/kwiver/blob/master/LICENSE for details.

/**
 * \file
 * \brief Implementation of \link kwiver::vital::track track \endlink
 */

#include "track.h"

#include <algorithm>
#include <stdexcept>

#include <vital/exceptions.h>

namespace {

class compare_state_frame
{
public:
  bool operator()( const kwiver::vital::track_state_sptr& ts, kwiver::vital::frame_id_t frame )
  {
    return ts && ts->frame() < frame;
  }
};

}

namespace kwiver {
namespace vital {

/// Default Constructor
track
::track(track_data_sptr d)
  : id_( invalid_track_id )
  , data_(d)
{
}

/// Copy Constructor
track
::track( const track& other )
  : history_()
  , id_( other.id_ )
  , data_( other.data_ )
{
}

/// Factory function
track_sptr
track
::create( track_data_sptr data )
{
  return track_sptr( new track( data ) );
}

/// Clone
track_sptr
track
::clone( clone_type ct ) const
{
  track_sptr t( new track( *this ) );
  t->history_.reserve( this->history_.size() );
  for( auto const& ts : this->history_ )
  {
    auto new_state = ts->clone( ct );
    new_state->track_ = t;
    t->history_.emplace_back( std::move( new_state ) );
  }
  return t;
}

/// Access the first frame number covered by this track
frame_id_t
track
::first_frame() const
{
  if( this->history_.empty() )
  {
    return 0;
  }
  return( *this->history_.begin() )->frame();
}

/// Access the last frame number covered by this track
frame_id_t
track
::last_frame() const
{
  if( this->history_.empty() )
  {
    return 0;
  }
  return( *this->history_.rbegin() )->frame();
}

/// Append a track state.
bool
track
::append( track_state_sptr&& state )
{
  if ( state && ! state->track_.expired() )
  {
    throw std::logic_error( "track states may not be reparented" );
  }

  if ( ! state ||
       ( ! this->history_.empty() &&
       ( this->last_frame() >= state->frame() ) ) )
  {
    return false;
  }
  state->track_ = this->shared_from_this();
  this->history_.emplace_back( std::move( state ) );
  return true;
}

/// Append an entire other track.
bool
track
::append( track& to_append )
{
  if ( ! this->history_.empty() && ! to_append.empty() &&
       ( this->last_frame() >= to_append.first_frame() ) )
  {
    return false;
  }
  for( auto ts : to_append.history_ )
  {
    ts->track_ = this->shared_from_this();
    this->history_.push_back(ts);
  }
  to_append.history_.clear();
  to_append.data_ = std::make_shared<track_data_redirect>(
                        this->shared_from_this(),
                        to_append.data_ );
  return true;
}

/// Insert a track state.
bool
track
::insert( track_state_sptr&& state )
{
  if ( ! state )
  {
    return false;
  }

  if ( ! state->track_.expired() )
  {
    throw std::logic_error( "track states may not be reparented" );
  }

  auto pos = std::lower_bound( this->history_.begin(), this->history_.end(),
                               state->frame(), compare_state_frame() );
  if( pos != this->history_.end() && (*pos)->frame() == state->frame() )
  {
    return false;
  }

  state->track_ = this->shared_from_this();
  this->history_.emplace( pos, std::move( state ) );
  return true;
}

/// Remove a track state
bool
track
::remove( track_state_sptr const& state )
{
  if ( !state )
  {
    return false;
  }

  auto pos = std::lower_bound(this->history_.begin(), this->history_.end(),
    state->frame(), compare_state_frame());

  if (pos == this->history_.end() || (*pos)->frame() != state->frame())
  {
    return false;
  }

  (*pos)->track_.reset();
  this->history_.erase(pos);
  return true;
}

/// Remove all track states
void
track
::clear()
{
  for (auto& s : this->history_)
  {
    s->track_.reset();
  }
  this->history_.clear();
}

/// Find the track state iterator matching \a frame
track::history_const_itr
track
::find( frame_id_t frame ) const
{
  if ( ( frame < this->first_frame() ) ||
       ( frame > this->last_frame() ) )
  {
    return this->end();
  }
  history_const_itr it = std::lower_bound( this->begin(), this->end(),
                                           frame, compare_state_frame() );
  if ( ( it != this->end() ) && ( (*it)->frame() == frame ) )
  {
    return it;
  }
  return this->end();
}

/// Return the set of all frame IDs covered by this track
std::set< frame_id_t >
track
::all_frame_ids() const
{
  std::set< frame_id_t > ids;

  for( track_state_sptr const ts : this->history_ )
  {
    ids.insert( ts->frame() );
  }
  return ids;
}

} } // end namespace vital
