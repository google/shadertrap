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

#ifndef LIBSHADERTRAP_VALUES_SEGMENT_H
#define LIBSHADERTRAP_VALUES_SEGMENT_H

#include <cinttypes>
#include <vector>

namespace shadertrap {

class ValuesSegment {
 public:
  explicit ValuesSegment(const std::vector<uint8_t>& byte_data);

  explicit ValuesSegment(const std::vector<float>& float_data);

  explicit ValuesSegment(const std::vector<int32_t>& int_data);

  explicit ValuesSegment(const std::vector<uint32_t>& uint_data);

  size_t GetSizeBytes() const { return data_.size(); }

  const std::vector<uint8_t>& GetData() const { return data_; }

 private:
  enum class ElementType { kByte, kFloat, kInt, kUint };

  ElementType element_type_;
  std::vector<uint8_t> data_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_VALUES_SEGMENT_H
