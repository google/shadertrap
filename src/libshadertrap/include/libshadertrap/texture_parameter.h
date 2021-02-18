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

#ifndef LIBSHADERTRAP_TEXTURE_PARAMETER_H
#define LIBSHADERTRAP_TEXTURE_PARAMETER_H

enum class TextureParameter { kMagFilter, kMinFilter };

enum class TextureParameterValue { kNearest, kLinear };

#endif  // LIBSHADERTRAP_TEXTURE_PARAMETER_H
