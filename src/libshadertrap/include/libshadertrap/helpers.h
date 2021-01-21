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

#ifndef LIBSHADERTRAP_HELPERS_H
#define LIBSHADERTRAP_HELPERS_H

#include <glad/glad.h>

#include <cstdlib>
#include <string>

namespace shadertrap {

std::string OpenglErrorString(GLenum err);

void PrintShaderError(GLuint shader);

void PrintProgramError(GLuint program);

#define errcode_crash(errcode, ...)                             \
  do {                                                          \
    printf("%s:%d (%s) ERROR: ", __FILE__, __LINE__, __func__); \
    printf(__VA_ARGS__);                                        \
    printf("\n");                                               \
    exit(errcode);                                              \
  } while (0)

#define crash(...)                                              \
  do {                                                          \
    printf("%s:%d (%s) ERROR: ", __FILE__, __LINE__, __func__); \
    printf(__VA_ARGS__);                                        \
    printf("\n");                                               \
    exit(EXIT_FAILURE);                                         \
  } while (0)

#define GL_CHECKERR(strfunc)                   \
  do {                                         \
    GLenum __err = glGetError();               \
    if (__err != GL_NO_ERROR) {                \
      crash("OpenGL error: %s(): %s", strfunc, \
            OpenglErrorString(__err).c_str()); \
    }                                          \
  } while (0)

#define GL_SAFECALL(func, ...) \
  do {                         \
    func(__VA_ARGS__);         \
    GL_CHECKERR(#func);        \
  } while (0)

#define GL_SAFECALL_NO_ARGS(func) \
  do {                            \
    func();                       \
    GL_CHECKERR(#func);           \
  } while (0)

#define COMPILE_ERROR_EXIT_CODE (101)
#define LINK_ERROR_EXIT_CODE (102)

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_HELPERS_H
