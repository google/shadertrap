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

cmake_minimum_required(VERSION 3.13)

##
## Project: glslang
##
## Provides targets: glslang.
##
if(NOT TARGET glslang)
    if(NOT IS_DIRECTORY ${SHADERTRAP_GLSLANG_REPO_DIR})
        message(
                FATAL_ERROR
                "Could not find glslang at ${SHADERTRAP_GLSLANG_REPO_DIR}. "
                "Try fetching submodules or set SHADERTRAP_GLSLANG_REPO_DIR.")
    endif()

    set(
        SKIP_GLSLANG_INSTALL
        ON
        CACHE
        BOOL
        "Skip installation"
    )

    add_subdirectory(${SHADERTRAP_GLSLANG_REPO_DIR} EXCLUDE_FROM_ALL)
endif()
