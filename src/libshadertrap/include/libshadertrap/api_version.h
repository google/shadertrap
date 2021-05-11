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

struct ApiVersion {
  enum class Api { GL, GLES } api;
  uint32_t major;
  uint32_t minor;

  ApiVersion(Api apiParam, uint32_t majorParam, uint32_t minorParam)
      : api(apiParam), major(majorParam), minor(minorParam) {}

  bool operator==(const ApiVersion& other) const {
    return api == other.api && major == other.major && minor == other.minor;
  }

  bool operator!=(const ApiVersion& other) const { return !(*this == other); }

  bool operator>=(const ApiVersion& other) const {
    return api == other.api && (major > other.major ||
                                (major == other.major && minor >= other.minor));
  }

  bool operator>(const ApiVersion& other) const {
    return *this >= other && *this != other;
  }

  bool operator<=(const ApiVersion& other) const { return other >= *this; }

  bool operator<(const ApiVersion& other) const {
    return *this > other && *this != other;
  }
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_API_VERSION_H
