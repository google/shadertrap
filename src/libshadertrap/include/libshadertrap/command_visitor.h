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

#ifndef LIBSHADERTRAP_COMMAND_VISITOR_H
#define LIBSHADERTRAP_COMMAND_VISITOR_H

#include "libshadertrap/command_assert_equal.h"
#include "libshadertrap/command_assert_pixels.h"
#include "libshadertrap/command_assert_similar_emd_histogram.h"
#include "libshadertrap/command_bind_sampler.h"
#include "libshadertrap/command_bind_shader_storage_buffer.h"
#include "libshadertrap/command_bind_texture.h"
#include "libshadertrap/command_bind_uniform_buffer.h"
#include "libshadertrap/command_compile_shader.h"
#include "libshadertrap/command_create_buffer.h"
#include "libshadertrap/command_create_empty_texture_2d.h"
#include "libshadertrap/command_create_program.h"
#include "libshadertrap/command_create_renderbuffer.h"
#include "libshadertrap/command_create_sampler.h"
#include "libshadertrap/command_declare_shader.h"
#include "libshadertrap/command_dump_buffer_binary.h"
#include "libshadertrap/command_dump_buffer_text.h"
#include "libshadertrap/command_dump_renderbuffer.h"
#include "libshadertrap/command_run_compute.h"
#include "libshadertrap/command_run_graphics.h"
#include "libshadertrap/command_set_sampler_parameter.h"
#include "libshadertrap/command_set_texture_parameter.h"
#include "libshadertrap/command_set_uniform.h"
#include "libshadertrap/shadertrap_program.h"

namespace shadertrap {

class CommandVisitor {
 public:
  CommandVisitor() = default;

  CommandVisitor(const CommandVisitor&) = delete;

  CommandVisitor& operator=(const CommandVisitor&) = delete;

  CommandVisitor(CommandVisitor&&) = delete;

  CommandVisitor& operator=(CommandVisitor&&) = delete;

  virtual ~CommandVisitor();

  bool VisitCommands(ShaderTrapProgram* shader_trap_program);

  virtual bool VisitAssertEqual(CommandAssertEqual* assert_equal) = 0;

  virtual bool VisitAssertPixels(CommandAssertPixels* assert_pixels) = 0;

  virtual bool VisitAssertSimilarEmdHistogram(
      CommandAssertSimilarEmdHistogram* assert_similar_emd_histogram) = 0;

  virtual bool VisitBindSampler(CommandBindSampler* bind_sampler) = 0;

  virtual bool VisitBindShaderStorageBuffer(
      CommandBindShaderStorageBuffer* bind_shader_storage_buffer) = 0;

  virtual bool VisitBindTexture(CommandBindTexture* bind_texture) = 0;

  virtual bool VisitBindUniformBuffer(
      CommandBindUniformBuffer* bind_uniform_buffer) = 0;

  virtual bool VisitCompileShader(CommandCompileShader* compile_shader) = 0;

  virtual bool VisitCreateBuffer(CommandCreateBuffer* create_buffer) = 0;

  virtual bool VisitCreateSampler(CommandCreateSampler* create_sampler) = 0;

  virtual bool VisitCreateEmptyTexture2D(
      CommandCreateEmptyTexture2D* create_empty_texture_2d) = 0;

  virtual bool VisitCreateProgram(CommandCreateProgram* create_program) = 0;

  virtual bool VisitCreateRenderbuffer(
      CommandCreateRenderbuffer* create_renderbuffer) = 0;

  virtual bool VisitDeclareShader(CommandDeclareShader* declare_shader) = 0;

  virtual bool VisitDumpBufferBinary(
      CommandDumpBufferBinary* dump_buffer_binary) = 0;

  virtual bool VisitDumpBufferText(CommandDumpBufferText* dump_buffer_text) = 0;

  virtual bool VisitDumpRenderbuffer(
      CommandDumpRenderbuffer* dump_renderbuffer) = 0;

  virtual bool VisitRunCompute(CommandRunCompute* run_compute) = 0;

  virtual bool VisitRunGraphics(CommandRunGraphics* run_graphics) = 0;

  virtual bool VisitSetSamplerParameter(
      CommandSetSamplerParameter* set_sampler_parameter) = 0;

  virtual bool VisitSetTextureParameter(
      CommandSetTextureParameter* set_texture_parameter) = 0;

  virtual bool VisitSetUniform(CommandSetUniform* set_uniform) = 0;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_VISITOR_H
