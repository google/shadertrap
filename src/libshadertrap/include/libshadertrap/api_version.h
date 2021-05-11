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

#ifndef LIBSHADERTRAP_API_VERSION_H
#define LIBSHADERTRAP_API_VERSION_H

namespace shadertrap {

class ApiVersion {
 public:
  enum class Api { GL, GLES };

  ApiVersion(Api api, uint32_t major_version, uint32_t minor_version)
      : api_(api),
        major_version_(major_version),
        minor_version_(minor_version) {}

  Api GetApi() const { return api_; }

  uint32_t GetMajorVersion() const { return major_version_; }

  uint32_t GetMinorVersion() const { return minor_version_; }

  bool operator==(const ApiVersion& other) const {
    return api_ == other.api_ && major_version_ == other.major_version_ &&
           minor_version_ == other.minor_version_;
  }

  bool operator!=(const ApiVersion& other) const { return !(*this == other); }

  bool operator>=(const ApiVersion& other) const {
    return api_ == other.api_ && (major_version_ > other.major_version_ ||
                                  (major_version_ == other.major_version_ &&
                                   minor_version_ >= other.minor_version_));
  }

  bool operator>(const ApiVersion& other) const {
    return *this >= other && *this != other;
  }

  bool operator<=(const ApiVersion& other) const { return other >= *this; }

  bool operator<(const ApiVersion& other) const { return other > *this; }

 private:
  Api api_;
  uint32_t major_version_;
  uint32_t minor_version_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_API_VERSION_H
