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

#include "libshadertrap/command_create_buffer.h"

#include <cassert>
#include <cstring>
#include <utility>

#include "libshadertrap/command_visitor.h"

namespace shadertrap {

CommandCreateBuffer::CommandCreateBuffer(std::unique_ptr<Token> start_token,
                                         std::string buffer_identifier,
                                         uint32_t size_bytes,
                                         const std::vector<uint8_t>& byte_data)
    : Command(std::move(start_token)),
      buffer_identifier_(std::move(buffer_identifier)),
      size_bytes_(size_bytes),
      has_initial_data_(true),
      initial_data_type_(InitialDataType::kByte) {
  assert(size_bytes == byte_data.size() && "Size mismatch.");
  initial_data_.resize(size_bytes);
  memcpy(initial_data_.data(), byte_data.data(), size_bytes);
}

CommandCreateBuffer::CommandCreateBuffer(std::unique_ptr<Token> start_token,
                                         std::string buffer_identifier,
                                         uint32_t size_bytes,
                                         const std::vector<float>& float_data)
    : Command(std::move(start_token)),
      buffer_identifier_(std::move(buffer_identifier)),
      size_bytes_(size_bytes),
      has_initial_data_(true),
      initial_data_type_(InitialDataType::kFloat) {
  assert(size_bytes == sizeof(float) * float_data.size() && "Size mismatch.");
  initial_data_.resize(size_bytes);
  memcpy(initial_data_.data(), float_data.data(), size_bytes);
}

CommandCreateBuffer::CommandCreateBuffer(std::unique_ptr<Token> start_token,
                                         std::string buffer_identifier,
                                         uint32_t size_bytes,
                                         const std::vector<int32_t>& int_data)
    : Command(std::move(start_token)),
      buffer_identifier_(std::move(buffer_identifier)),
      size_bytes_(size_bytes),
      has_initial_data_(true),
      initial_data_type_(InitialDataType::kInt) {
  assert(size_bytes == sizeof(int32_t) * int_data.size() && "Size mismatch.");
  initial_data_.resize(size_bytes);
  memcpy(initial_data_.data(), int_data.data(), size_bytes);
}

CommandCreateBuffer::CommandCreateBuffer(std::unique_ptr<Token> start_token,
                                         std::string buffer_identifier,
                                         uint32_t size_bytes,
                                         const std::vector<uint32_t>& uint_data)
    : Command(std::move(start_token)),
      buffer_identifier_(std::move(buffer_identifier)),
      size_bytes_(size_bytes),
      has_initial_data_(true),
      initial_data_type_(InitialDataType::kUint) {
  assert(size_bytes == sizeof(uint32_t) * uint_data.size() && "Size mismatch.");
  initial_data_.resize(size_bytes);
  memcpy(initial_data_.data(), uint_data.data(), size_bytes);
}

bool CommandCreateBuffer::Accept(CommandVisitor* visitor) {
  return visitor->VisitCreateBuffer(this);
}

}  // namespace shadertrap
