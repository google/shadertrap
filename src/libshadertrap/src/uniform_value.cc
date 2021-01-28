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

#include "libshadertrap/uniform_value.h"

#include <cstring>

UniformValue::UniformValue(ElementType element_type,
                           const std::vector<int32_t>& int_data)
    : element_type_(element_type), maybe_array_size_(false, 0U) {
  data_.resize(sizeof(int32_t) * int_data.size());
  memcpy(data_.data(), int_data.data(), data_.size());
}

UniformValue::UniformValue(ElementType element_type,
                           const std::vector<int32_t>& int_data,
                           size_t array_size)
    : element_type_(element_type), maybe_array_size_(true, array_size) {
  data_.resize(sizeof(int32_t) * int_data.size());
  memcpy(data_.data(), int_data.data(), data_.size());
}

UniformValue::UniformValue(ElementType element_type,
                           const std::vector<uint32_t>& uint_data)
    : element_type_(element_type), maybe_array_size_(false, 0U) {
  data_.resize(sizeof(uint32_t) * uint_data.size());
  memcpy(data_.data(), uint_data.data(), data_.size());
}

UniformValue::UniformValue(ElementType element_type,
                           const std::vector<uint32_t>& uint_data,
                           size_t array_size)
    : element_type_(element_type), maybe_array_size_(true, array_size) {
  data_.resize(sizeof(uint32_t) * uint_data.size());
  memcpy(data_.data(), uint_data.data(), data_.size());
}

UniformValue::UniformValue(ElementType element_type,
                           const std::vector<float>& float_data)
    : element_type_(element_type), maybe_array_size_(false, 0U) {
  data_.resize(sizeof(float) * float_data.size());
  memcpy(data_.data(), float_data.data(), data_.size());
}

UniformValue::UniformValue(ElementType element_type,
                           const std::vector<float>& float_data,
                           size_t array_size)
    : element_type_(element_type), maybe_array_size_(true, array_size) {
  data_.resize(sizeof(float) * float_data.size());
  memcpy(data_.data(), float_data.data(), data_.size());
}
