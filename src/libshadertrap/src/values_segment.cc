// Copyright 2021 The ShaderTrap Project Authors
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

#include "libshadertrap/values_segment.h"

#include <cstring>
#include <utility>

namespace shadertrap {

ValuesSegment::ValuesSegment(std::vector<uint8_t> byte_data)
    : element_type_(ElementType::kByte), data_(std::move(byte_data)) {}

ValuesSegment::ValuesSegment(const std::vector<float>& float_data)
    : element_type_(ElementType::kFloat) {
  data_.resize(float_data.size() * sizeof(float));
  memcpy(data_.data(), float_data.data(), data_.size());
}

ValuesSegment::ValuesSegment(const std::vector<int32_t>& int_data)
    : element_type_(ElementType::kInt) {
  data_.resize(int_data.size() * sizeof(int32_t));
  memcpy(data_.data(), int_data.data(), data_.size());
}

ValuesSegment::ValuesSegment(const std::vector<uint32_t>& uint_data)
    : element_type_(ElementType::kUint) {
  data_.resize(uint_data.size() * sizeof(uint32_t));
  memcpy(data_.data(), uint_data.data(), data_.size());
}

}  // namespace shadertrap
