# Copyright 2021 The ShaderTrap Project Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if(SHADERTRAP_WARNING_SUPPRESSIONS)
    if(MSVC)
        add_compile_options(
                /wd5039
        )
    else()
    endif()
endif()

set(GET_GL_FUNCTIONS_GENERATOR_SOURCE ${CMAKE_SOURCE_DIR}/src/generators/generate_get_gl_functions.py)
set(OPENGL_XML ${SHADERTRAP_OPENGL_REGISTRY_REPO_DIR}/xml/gl.xml)

add_custom_command(
        OUTPUT src/get_gl_functions.cc
        COMMAND ${CMAKE_COMMAND} -E make_directory src
        COMMAND python ${GET_GL_FUNCTIONS_GENERATOR_SOURCE} ${OPENGL_XML} ${CMAKE_CURRENT_BINARY_DIR}/src
        DEPENDS ${GET_GL_FUNCTIONS_GENERATOR_SOURCE} ${OPENGL_XML}
        COMMENT "Generate code that populates struct of functions based on the OpenGL registry."
)

add_executable(shadertrap
        include_private/include/shadertrap/get_gl_functions.h

        src/main.cc
        ${CMAKE_CURRENT_BINARY_DIR}/src/get_gl_functions.cc
)
target_include_directories(shadertrap PRIVATE include_private/include)
target_link_libraries(shadertrap PRIVATE egl_headers glad glslang libshadertrap ${SHADERTRAP_EGL_LIB_PATH})
