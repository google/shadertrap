# Copyright 2020 The ShaderTrap Project Authors
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

project(shadertrap
    VERSION 1.0
    DESCRIPTION "ShaderTrap"
    LANGUAGES CXX)

# Note: this is not an option; it is used to set the default value for some options below.
# It is set to ON if shadertrap is the root project being built, and OFF otherwise.
set(SHADERTRAP_PROJECT_IS_ROOT OFF)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(SHADERTRAP_PROJECT_IS_ROOT ON)
endif()


# Options (i.e. cache variables) defined via "option" (if ON/OFF) or "set" (if not ON/OFF):
set(
    SHADERTRAP_GOOGLETEST_REPO_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/googletest/googletest
    CACHE
    PATH
    "Path to a https://github.com/google/googletest repo.")

set(
    SHADERTRAP_GLSLANG_REPO_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glslang/glslang
    CACHE
    PATH
    "Path to a https://github.com/KhronosGroup/glslang repo.")

set(
    SHADERTRAP_EGL_REGISTRY_REPO_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/EGL-Registry/EGL-Registry
    CACHE
    PATH
    "Path to a https://github.com/KhronosGroup/EGL-Registry repo.")

set(
    SHADERTRAP_OPENGL_REGISTRY_REPO_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/OpenGL-Registry/OpenGL-Registry
    CACHE
    PATH
    "Path to a https://github.com/KhronosGroup/OpenGL-Registry repo.")

option(
    SHADERTRAP_USE_LLVM_LIBCPP
    "Use LLVM's libc++ when using Clang, plus various other LLVM options, but only if shadertrap is the root project."
    OFF)

option(
    SHADERTRAP_WARNINGS_AS_ERRORS
    "Enable warnings as errors for shadertrap targets."
    ${SHADERTRAP_PROJECT_IS_ROOT})

option(
    SHADERTRAP_WARNINGS_EXTRA
    "Enable extra warnings via compile flags for shadertrap targets."
    ${SHADERTRAP_PROJECT_IS_ROOT})

option(
    SHADERTRAP_WARNING_SUPPRESSIONS
    "Enable suppression of specific warnings for shadertrap targets."
    ${SHADERTRAP_PROJECT_IS_ROOT})

option(
    SHADERTRAP_BUILD_TESTING
    "Build tests."
    ${SHADERTRAP_PROJECT_IS_ROOT})

option(
    SHADERTRAP_DEQP
    "Enable for dEQP integration"
    OFF)

option(
    SHADERTRAP_SKIP_EXECUTABLE
    "Skip building executable along with the library"
    ${SHADERTRAP_DEQP})

option(
    SHADERTRAP_SKIP_LODEPNG
    "Skip building lodepng"
    ${SHADERTRAP_DEQP})


# TODO(paulthomson): Remove this eventually.
find_library(SHADERTRAP_EGL_LIB_PATH NAMES EGL libEGL)

if(SHADERTRAP_PROJECT_IS_ROOT)
    # Set some global compiler flags.
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    include(CTest)

    # SHADERTRAP_USE_LLVM_LIBCPP=ON means we will statically link (if using non-Android Clang):
    #  - libc++
    #  - compiler-rt
    #  - libc++abi
    #  - libunwind
    # This is very useful for getting binaries that only depend on libc.
    if(
        SHADERTRAP_USE_LLVM_LIBCPP AND
        CMAKE_CXX_COMPILER_ID STREQUAL Clang AND
        NOT ANDROID)

        # TODO(paulthomson): We may want to build a custom version of Clang instead of messing with its options.

        # Use LLVM's libc++ instead of GCC's libstdc++.
        add_compile_options(-stdlib=libc++)

        # Link options:
        # Use LLVM's libc++ instead of GCC's libstdc++.
        add_link_options(-stdlib=libc++)
        # Use LLVM's lld as the linker.
        add_link_options(-fuse-ld=lld)
        # Statically link the C++ library (which is not GCC's libstdc++ because of the -stdlib flag above).
        add_link_options(-static-libstdc++)
        # Use LLVM's compiler-rt instead of libgcc, which will be statically linked.
        add_link_options(--rtlib=compiler-rt)

        # Include the path to LLVM's libc++abi.a
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} --print-file-name=libc++abi.a
            OUTPUT_VARIABLE SHADERTRAP_TEMP_CPP_ABI_PATH
        )
        string(REGEX REPLACE "\n" "" SHADERTRAP_TEMP_CPP_ABI_PATH ${SHADERTRAP_TEMP_CPP_ABI_PATH})

        add_link_options(${SHADERTRAP_TEMP_CPP_ABI_PATH})

        # Include the path to LLVM's libunwind.a
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} --print-file-name=libunwind.a
            OUTPUT_VARIABLE SHADERTRAP_TEMP_UNWIND_PATH
        )
        string(REGEX REPLACE "\n" "" SHADERTRAP_TEMP_UNWIND_PATH ${SHADERTRAP_TEMP_UNWIND_PATH})

        add_link_options(${SHADERTRAP_TEMP_UNWIND_PATH})
    endif()
endif()


## Third party targets.

add_subdirectory(third_party/EGL-Registry EXCLUDE_FROM_ALL)

add_subdirectory(third_party/OpenGL-Registry EXCLUDE_FROM_ALL)

# TODO(paulthomson): Probably remove this and use a different loading strategy/library.
add_subdirectory(third_party/glad EXCLUDE_FROM_ALL)

if(NOT SHADERTRAP_SKIP_LODEPNG)
    add_subdirectory(third_party/lodepng EXCLUDE_FROM_ALL)
endif()

if(SHADERTRAP_BUILD_TESTING)
    add_subdirectory(third_party/googletest)
endif()

add_subdirectory(third_party/glslang)


## The targets that follow are all shadertrap targets.
## We now conditionally set some compile options related to warnings.
## Most of these will only be set if shadertrap is the root project being built.
## Using "add_*" functions is generally discouraged, but is suitable for
## "global" compile options. These will propagate to subdirectories via
## add_subdirectory(...) and will not be PUBLIC compile options (i.e. they
## will NOT propagate to targets above this project that link with these
## targets.

if(SHADERTRAP_WARNINGS_AS_ERRORS)
    if(MSVC)
        add_compile_options(/WX)
    else()
        add_compile_options(-Werror)
    endif()
endif()

if(SHADERTRAP_WARNINGS_EXTRA)
    if(MSVC)
        add_compile_options(/Wall)
    else()
        add_compile_options(-Wall -Wextra -pedantic)
        if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
            add_compile_options(-Weverything)
        endif()
    endif()
endif()

if(SHADERTRAP_WARNING_SUPPRESSIONS)
    if(MSVC)
        add_compile_options(
            # Treat angle bracket headers as external headers.
            #/external:anglebrackets

            # Disable warnings from external headers.
            #/external:W0

            # Disable specific warnings:
            /wd4068  # warning C4068: unknown pragma
            /wd4820  # warning C4820: 'xxx': 'x' bytes padding added after data member 'xxx'
            /wd4191  # warning C4191: 'reinterpret_cast': unsafe conversion from 'x' to 'x'
            /wd4625  # copy constructor was implicitly defined as deleted
            /wd5026  # move constructor was implicitly defined as deleted
            /wd4626  # assignment operator was implicitly defined as deleted
            /wd5027  # move assignment operator was implicitly defined as deleted
            /wd4514  # warning C4514: 'x': unreferenced inline function has been removed
            /wd4711  # warning C4711: function 'x' selected for automatic inline expansion (informational)
            /wd4710  # warning C4710: 'x': function not inlined (informational)
            /wd4996  # warning C4996: 'getenv': This function or variable may be unsafe.
            /wd5045  # warning C5045: Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
            /wd4061  # warning C4061: enumerator 'identifier' in switch of enum 'enumeration' is not explicitly handled by a case label
            /wd4868  # warning C4868: compiler may not enforce left-to-right evaluation order in braced initializer list
        )
    else()
        add_compile_options(
            -Wno-unknown-pragmas
            -Wno-unknown-warning-option
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-padded
            -Wno-switch-enum
        )
    endif()
endif()

if(SHADERTRAP_DEQP)
    add_compile_definitions(SHADERTRAP_DEQP)
endif()

if(NOT SHADERTRAP_SKIP_LODEPNG)
    add_compile_definitions(SHADERTRAP_LODEPNG)
endif()

add_subdirectory(src/libshadertrap)

if(SHADERTRAP_BUILD_TESTING)
    add_subdirectory(src/libshadertraptest)
endif()

if(NOT SHADERTRAP_SKIP_EXECUTABLE)
    add_subdirectory(src/shadertrap)
endif()
