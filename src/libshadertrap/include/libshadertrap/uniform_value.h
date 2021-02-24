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

#ifndef LIBSHADERTRAP_UNIFORM_VALUE_H
#define LIBSHADERTRAP_UNIFORM_VALUE_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

class UniformValue {
 public:
  enum class ElementType {
    kFloat,
    kVec2,
    kVec3,
    kVec4,
    kInt,
    kIvec2,
    kIvec3,
    kIvec4,
    kUint,
    kUvec2,
    kUvec3,
    kUvec4,
    kMat2x2,
    kMat2x3,
    kMat2x4,
    kMat3x2,
    kMat3x3,
    kMat3x4,
    kMat4x2,
    kMat4x3,
    kMat4x4,
    kSampler2d
  };

  UniformValue(ElementType element_type, const std::vector<float>& float_data);
  UniformValue(ElementType element_type, const std::vector<float>& float_data,
               size_t array_size);

  UniformValue(ElementType element_type, const std::vector<int32_t>& int_data);
  UniformValue(ElementType element_type, const std::vector<int32_t>& int_data,
               size_t array_size);

  UniformValue(ElementType element_type,
               const std::vector<uint32_t>& uint_data);
  UniformValue(ElementType element_type, const std::vector<uint32_t>& uint_data,
               size_t array_size);

  ElementType GetElementType() const { return element_type_; }

  bool IsArray() const { return maybe_array_size_.first; }

  size_t GetArraySize() const {
    assert(IsArray() && "Attempt to query array size of non-array.");
    return maybe_array_size_.second;
  }

  const float* GetFloatData() const {
    // TODO(afd) Assert that the data is floating-point.
    return reinterpret_cast<const float*>(data_.data());
  }

  const int32_t* GetIntData() const {
    // TODO(afd) Assert that the data is integer.
    return reinterpret_cast<const int32_t*>(data_.data());
  }

  const uint32_t* GetUintData() const {
    // TODO(afd) Assert that the data is unsigned integer.
    return reinterpret_cast<const uint32_t*>(data_.data());
  }

 private:
  ElementType element_type_;
  // (false, 0) if there is no array size, otherwise (true, array_size).
  std::pair<bool, size_t> maybe_array_size_;
  std::vector<char> data_;
};

#endif  // LIBSHADERTRAP_UNIFORM_VALUE_H
