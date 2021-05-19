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

add_library(libshadertrap STATIC
        include/libshadertrap/api_version.h
        include/libshadertrap/checker.h
        include/libshadertrap/command.h
        include/libshadertrap/command_assert_equal.h
        include/libshadertrap/command_assert_pixels.h
        include/libshadertrap/command_assert_similar_emd_histogram.h
        include/libshadertrap/command_bind_sampler.h
        include/libshadertrap/command_bind_shader_storage_buffer.h
        include/libshadertrap/command_bind_texture.h
        include/libshadertrap/command_bind_uniform_buffer.h
        include/libshadertrap/command_compile_shader.h
        include/libshadertrap/command_create_buffer.h
        include/libshadertrap/command_create_empty_texture_2d.h
        include/libshadertrap/command_create_program.h
        include/libshadertrap/command_create_renderbuffer.h
        include/libshadertrap/command_create_sampler.h
        include/libshadertrap/command_declare_shader.h
        include/libshadertrap/command_dump_buffer_binary.h
        include/libshadertrap/command_dump_buffer_text.h
        include/libshadertrap/command_dump_renderbuffer.h
        include/libshadertrap/command_run_compute.h
        include/libshadertrap/command_run_graphics.h
        include/libshadertrap/command_set_sampler_parameter.h
        include/libshadertrap/command_set_texture_parameter.h
        include/libshadertrap/command_set_uniform.h
        include/libshadertrap/command_visitor.h
        include/libshadertrap/compound_visitor.h
        include/libshadertrap/executor.h
        include/libshadertrap/gl_functions.h
        include/libshadertrap/glslang.h
        include/libshadertrap/make_unique.h
        include/libshadertrap/message_consumer.h
        include/libshadertrap/parser.h
        include/libshadertrap/shadertrap_program.h
        include/libshadertrap/texture_parameter.h
        include/libshadertrap/token.h
        include/libshadertrap/tokenizer.h
        include/libshadertrap/uniform_value.h
        include/libshadertrap/values_segment.h
        include/libshadertrap/vertex_attribute_info.h

        src/checker.cc
        src/command.cc
        src/command_assert_equal.cc
        src/command_assert_pixels.cc
        src/command_assert_similar_emd_histogram.cc
        src/command_bind_sampler.cc
        src/command_bind_shader_storage_buffer.cc
        src/command_bind_texture.cc
        src/command_bind_uniform_buffer.cc
        src/command_compile_shader.cc
        src/command_create_buffer.cc
        src/command_create_empty_texture_2d.cc
        src/command_create_program.cc
        src/command_create_renderbuffer.cc
        src/command_create_sampler.cc
        src/command_declare_shader.cc
        src/command_dump_buffer_binary.cc
        src/command_dump_buffer_text.cc
        src/command_dump_renderbuffer.cc
        src/command_run_compute.cc
        src/command_run_graphics.cc
        src/command_set_sampler_parameter.cc
        src/command_set_texture_parameter.cc
        src/command_set_uniform.cc
        src/command_visitor.cc
        src/compound_visitor.cc
        src/executor.cc
        src/message_consumer.cc
        src/parser.cc
        src/shadertrap_program.cc
        src/token.cc
        src/tokenizer.cc
        src/uniform_value.cc
        src/values_segment.cc
        src/vertex_attribute_info.cc
        )

target_include_directories(libshadertrap PUBLIC include PRIVATE include_private/include)
target_link_libraries(libshadertrap PUBLIC shadertrap_egl_headers shadertrap_gl_headers PRIVATE glslang)
if(NOT SHADERTRAP_SKIP_LODEPNG)
    target_link_libraries(libshadertrap PRIVATE lodepng)
endif()

target_compile_features(libshadertrap PUBLIC cxx_std_11)
