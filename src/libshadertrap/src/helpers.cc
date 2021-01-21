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

#include "libshadertrap/helpers.h"

#include <iostream>
#include <vector>

namespace shadertrap {

std::string OpenglErrorString(GLenum err) {
  switch (err) {
    case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";
    case GL_STACK_OVERFLOW:
      return "GL_STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW:
      return "GL_STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";
    default:
      return "UNKNOW_ERROR";
  }
}

void PrintShaderError(GLuint shader) {
  GLint length = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
  // The length includes the NULL character
  std::vector<GLchar> error_log(static_cast<size_t>(length), 0);
  glGetShaderInfoLog(shader, length, &length, &error_log[0]);
  if (length > 0) {
    std::string s(&error_log[0]);
    std::cout << s << std::endl;
  }
}

void PrintProgramError(GLuint program) {
  GLint length = 0;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
  // The length includes the NULL character
  std::vector<GLchar> error_log(static_cast<size_t>(length), 0);
  glGetProgramInfoLog(program, length, &length, &error_log[0]);
  if (length > 0) {
    std::string s(&error_log[0]);
    std::cout << s << std::endl;
  }
}

}  // namespace shadertrap
