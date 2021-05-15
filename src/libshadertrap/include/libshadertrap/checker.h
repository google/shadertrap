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

#ifndef LIBSHADERTRAP_CHECKER_H
#define LIBSHADERTRAP_CHECKER_H

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include "libshadertrap/api_version.h"
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
#include "libshadertrap/command_visitor.h"
#include "libshadertrap/glslang.h"
#include "libshadertrap/message_consumer.h"
#include "libshadertrap/token.h"

namespace shadertrap {

class Checker : public CommandVisitor {
 public:
  Checker(MessageConsumer* message_consumer, ApiVersion api_version);

  bool VisitAssertEqual(CommandAssertEqual* assert_equal) override;

  bool VisitAssertPixels(CommandAssertPixels* assert_pixels) override;

  bool VisitAssertSimilarEmdHistogram(
      CommandAssertSimilarEmdHistogram* assert_similar_emd_histogram) override;

  bool VisitBindSampler(CommandBindSampler* bind_sampler) override;

  bool VisitBindShaderStorageBuffer(
      CommandBindShaderStorageBuffer* bind_shader_storage_buffer) override;

  bool VisitBindTexture(CommandBindTexture* bind_texture) override;

  bool VisitBindUniformBuffer(
      CommandBindUniformBuffer* bind_uniform_buffer) override;

  bool VisitCompileShader(CommandCompileShader* compile_shader) override;

  bool VisitCreateBuffer(CommandCreateBuffer* create_buffer) override;

  bool VisitCreateSampler(CommandCreateSampler* create_sampler) override;

  bool VisitCreateEmptyTexture2D(
      CommandCreateEmptyTexture2D* create_empty_texture_2d) override;

  bool VisitCreateProgram(CommandCreateProgram* create_program) override;

  bool VisitCreateRenderbuffer(
      CommandCreateRenderbuffer* create_renderbuffer) override;

  bool VisitDeclareShader(CommandDeclareShader* declare_shader) override;

  bool VisitDumpBufferBinary(
      CommandDumpBufferBinary* dump_buffer_binary) override;

  bool VisitDumpBufferText(CommandDumpBufferText* dump_buffer_text) override;

  bool VisitDumpRenderbuffer(
      CommandDumpRenderbuffer* dump_renderbuffer) override;

  bool VisitRunCompute(CommandRunCompute* run_compute) override;

  bool VisitRunGraphics(CommandRunGraphics* run_graphics) override;

  bool VisitSetSamplerParameter(
      CommandSetSamplerParameter* set_sampler_parameter) override;

  bool VisitSetTextureParameter(
      CommandSetTextureParameter* set_texture_parameter) override;

  bool VisitSetUniform(CommandSetUniform* set_uniform) override;

  bool CheckIdentifierIsFresh(const Token& identifier_token);

 private:
  // |glslang_output| is output from glslang, which may contain error messages
  // that use irrelevant file identifiers, and line numbers relative to the
  // beginning of the shader string that was parsed. This strips away the
  // irrelevant file identifiers and increments line numbers by |line_offset|
  // so that they are relative to the start of the ShaderTrap file being
  // processed.
  static std::string FixLinesInGlslangOutput(const std::string& glslang_output,
                                             size_t line_offset);

  // Requires that |renderbuffer_token_1| and |renderbuffer_token_2| refer to
  // renderbuffers. Returns true if and only if their widths and heights match.
  bool CheckRenderbufferDimensionsMatch(const Token& renderbuffer_token_1,
                                        const Token& renderbuffer_token_2);

  MessageConsumer* message_consumer_;
  ApiVersion api_version_;
  std::unordered_map<std::string, const Token&> used_identifiers_;
  std::unordered_map<std::string, CommandDeclareShader*> declared_shaders_;
  std::unordered_map<std::string, CommandCompileShader*> compiled_shaders_;
  std::unordered_map<std::string, CommandCreateProgram*> created_programs_;
  std::unordered_map<std::string, CommandCreateBuffer*> created_buffers_;
  std::unordered_map<std::string, CommandCreateRenderbuffer*>
      created_renderbuffers_;
  std::unordered_map<std::string, CommandCreateSampler*> created_samplers_;
  std::unordered_map<std::string, CommandCreateEmptyTexture2D*>
      created_textures_;
  std::unordered_map<std::string, std::unique_ptr<glslang::TShader>>
      glslang_shaders_;
  std::unordered_map<std::string, std::unique_ptr<glslang::TProgram>>
      glslang_programs_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_CHECKER_H
