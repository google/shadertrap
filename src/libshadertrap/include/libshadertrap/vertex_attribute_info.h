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

#ifndef LIBSHADERTRAP_VERTEX_ATTRIBUTE_INFO_H
#define LIBSHADERTRAP_VERTEX_ATTRIBUTE_INFO_H

#include <cstddef>
#include <memory>
#include <string>

#include "libshadertrap/token.h"

namespace shadertrap {

class VertexAttributeInfo {
 public:
  VertexAttributeInfo(std::unique_ptr<Token> buffer_identifier,
                      size_t offset_bytes, size_t stride_bytes,
                      size_t dimension);

  const std::string& GetBufferIdentifier() const {
    return buffer_identifier_->GetText();
  }

  const Token& GetBufferIdentifierToken() const { return *buffer_identifier_; }

  size_t GetOffsetBytes() const { return offset_bytes_; }

  size_t GetStrideBytes() const { return stride_bytes_; }

  size_t GetDimension() const { return dimension_; }

 private:
  std::unique_ptr<Token> buffer_identifier_;
  size_t offset_bytes_;
  size_t stride_bytes_;
  size_t dimension_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_VERTEX_ATTRIBUTE_INFO_H
