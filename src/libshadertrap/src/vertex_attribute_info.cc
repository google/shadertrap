// Copyright 2020 The ShaderTrap Project Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "libshadertrap/vertex_attribute_info.h"

#include <utility>

namespace shadertrap {

VertexAttributeInfo::VertexAttributeInfo(
    std::unique_ptr<Token> buffer_identifier, size_t offset_bytes,
    size_t stride_bytes, size_t dimension)
    : buffer_identifier_(std::move(buffer_identifier)),
      offset_bytes_(offset_bytes),
      stride_bytes_(stride_bytes),
      dimension_(dimension) {}

}  // namespace shadertrap
