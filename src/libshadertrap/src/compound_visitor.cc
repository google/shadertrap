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

#include "libshadertrap/compound_visitor.h"

#include <utility>

namespace shadertrap {

CompoundVisitor::CompoundVisitor(
    std::vector<std::unique_ptr<CommandVisitor>> visitors)
    : visitors_(std::move(visitors)) {}

bool CompoundVisitor::ApplyVisitors(Command* command) {
  for (auto& visitor : visitors_) {
    if (!command->Accept(visitor.get())) {
      return false;
    }
  }
  return true;
}

bool CompoundVisitor::VisitAssertEqual(CommandAssertEqual* assert_equal) {
  return ApplyVisitors(assert_equal);
}

bool CompoundVisitor::VisitAssertPixels(CommandAssertPixels* assert_pixels) {
  return ApplyVisitors(assert_pixels);
}

bool CompoundVisitor::VisitAssertSimilarEmdHistogram(
    CommandAssertSimilarEmdHistogram* assert_similar_emd_histogram) {
  return ApplyVisitors(assert_similar_emd_histogram);
}

bool CompoundVisitor::VisitBindSampler(CommandBindSampler* bind_sampler) {
  return ApplyVisitors(bind_sampler);
}

bool CompoundVisitor::VisitBindShaderStorageBuffer(
    CommandBindShaderStorageBuffer* bind_shader_storage_buffer) {
  return ApplyVisitors(bind_shader_storage_buffer);
}

bool CompoundVisitor::VisitBindTexture(CommandBindTexture* bind_texture) {
  return ApplyVisitors(bind_texture);
}

bool CompoundVisitor::VisitBindUniformBuffer(
    CommandBindUniformBuffer* bind_uniform_buffer) {
  return ApplyVisitors(bind_uniform_buffer);
}

bool CompoundVisitor::VisitCompileShader(CommandCompileShader* compile_shader) {
  return ApplyVisitors(compile_shader);
}

bool CompoundVisitor::VisitCreateBuffer(CommandCreateBuffer* create_buffer) {
  return ApplyVisitors(create_buffer);
}

bool CompoundVisitor::VisitCreateSampler(CommandCreateSampler* create_sampler) {
  return ApplyVisitors(create_sampler);
}

bool CompoundVisitor::VisitCreateEmptyTexture2D(
    CommandCreateEmptyTexture2D* create_empty_texture_2d) {
  return ApplyVisitors(create_empty_texture_2d);
}

bool CompoundVisitor::VisitCreateProgram(CommandCreateProgram* create_program) {
  return ApplyVisitors(create_program);
}

bool CompoundVisitor::VisitCreateRenderbuffer(
    CommandCreateRenderbuffer* create_renderbuffer) {
  return ApplyVisitors(create_renderbuffer);
}

bool CompoundVisitor::VisitDeclareShader(CommandDeclareShader* declare_shader) {
  return ApplyVisitors(declare_shader);
}

bool CompoundVisitor::VisitDumpBufferBinary(
    CommandDumpBufferBinary* dump_buffer_binary) {
  return ApplyVisitors(dump_buffer_binary);
}

bool CompoundVisitor::VisitDumpBufferText(
    CommandDumpBufferText* dump_buffer_text) {
  return ApplyVisitors(dump_buffer_text);
}

bool CompoundVisitor::VisitDumpRenderbuffer(
    CommandDumpRenderbuffer* dump_renderbuffer) {
  return ApplyVisitors(dump_renderbuffer);
}

bool CompoundVisitor::VisitRunCompute(CommandRunCompute* run_compute) {
  return ApplyVisitors(run_compute);
}

bool CompoundVisitor::VisitRunGraphics(CommandRunGraphics* run_graphics) {
  return ApplyVisitors(run_graphics);
}

bool CompoundVisitor::VisitSetSamplerParameter(
    CommandSetSamplerParameter* set_sampler_parameter) {
  return ApplyVisitors(set_sampler_parameter);
}

bool CompoundVisitor::VisitSetTextureParameter(
    CommandSetTextureParameter* set_texture_parameter) {
  return ApplyVisitors(set_texture_parameter);
}

bool CompoundVisitor::VisitSetUniform(CommandSetUniform* set_uniform) {
  return ApplyVisitors(set_uniform);
}

}  // namespace shadertrap
