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

if(SHADERTRAP_WARNING_SUPPRESSIONS)
    if(MSVC)
    else()
        add_compile_options(
                -Wno-global-constructors
        )
    endif()
endif()

add_executable(libshadertraptest
        include_private/include/libshadertraptest/collecting_message_consumer.h
        include_private/include/libshadertraptest/gtest.h

        src/checker_test.cc
        src/collecting_message_consumer.cc
        src/parser_test.cc
)
target_link_libraries(libshadertraptest PRIVATE glslang libshadertrap gtest_main)
target_include_directories(libshadertraptest PRIVATE include_private/include)

add_test(NAME libshadertraptest COMMAND libshadertraptest)
