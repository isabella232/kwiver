/*ckwg +29
 * Copyright 2018 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ARROWS_SERIALIZATION_PROTO_BOUNDING_BOX_H
#define ARROWS_SERIALIZATION_PROTO_BOUNDING_BOX_H

#include <arrows/serialize/protobuf/kwiver_serialize_proto_export.h>
#include <vital/algo/data_serializer.h>
#include <vital/types/bounding_box.h>
#include <vital/types/protobuf/bounding_box.pb.h>

namespace kwiver {
namespace arrows {
namespace serialize {
namespace proto {

class KWIVER_SERIALIZE_PROTO_EXPORT bounding_box
  : public vital::algorithm_impl< bounding_box, vital::algo::data_serializer >
{
public:
  // Type name this class supports
  static constexpr char const* name = "kwiver:bounding_box";

  static constexpr char const* description =
    "Serializes a bounding_box using protobuf notation. "
    "This implementation only handles a single data item.";

  bounding_box();
  virtual ~bounding_box();

  virtual std::shared_ptr< std::string > serialize( const serialize_param_t elements );
  virtual deserialize_result_t deserialize( std::shared_ptr< std::string > message );

  // Convert between native and protobuf formats
  static bool convert_protobuf( const ::vital::protobuf::bounding_box&  proto_bbox,
                                kwiver::vital::bounding_box_d& bbox );

  static bool convert_protobuf( const kwiver::vital::bounding_box_d& bbox,
                                ::vital::protobuf::bounding_box&  proto_bbox );
};

} } } }       // end namespace kwiver

#endif // ARROWS_SERIALIZATION_PROTO_BOUNDING_BOX_H
