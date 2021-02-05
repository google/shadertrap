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

#include "libshadertrap/checker.h"

#include <cstddef>

namespace shadertrap {

Checker::Checker(MessageConsumer* message_consumer)
    : message_consumer_(message_consumer) {}

bool Checker::VisitAssertEqual(CommandAssertEqual* command_assert_equal) {
  // TODO(afd): Either both arguments must be renderbuffers or both arguments
  //  must be buffers
  // TODO(afd): Both arguments must have the same dimensions
  (void)command_assert_equal;
  return true;
}

bool Checker::VisitAssertPixels(CommandAssertPixels* command_assert_pixels) {
  // TODO(afd): first argument must be a renderbuffer
  // TODO(afd): the rectangle must be in-bounds
  (void)command_assert_pixels;
  return true;
}

bool Checker::VisitAssertSimilarEmdHistogram(
    CommandAssertSimilarEmdHistogram* command_assert_similar_emd_histogram) {
  (void)command_assert_similar_emd_histogram;
  return true;
}

bool Checker::VisitBindSampler(CommandBindSampler* command_bind_sampler) {
  (void)command_bind_sampler;
  return true;
}

bool Checker::VisitBindStorageBuffer(
    CommandBindStorageBuffer* command_bind_storage_buffer) {
  (void)command_bind_storage_buffer;
  return true;
}

bool Checker::VisitBindTexture(CommandBindTexture* command_bind_texture) {
  (void)command_bind_texture;
  return true;
}

bool Checker::VisitBindUniformBuffer(
    CommandBindUniformBuffer* command_bind_uniform_buffer) {
  (void)command_bind_uniform_buffer;
  return true;
}

bool Checker::VisitCompileShader(CommandCompileShader* compile_shader) {
  if (!CheckIdentifierIsFresh(compile_shader->GetResultIdentifierToken())) {
    return false;
  }
  if (declared_shaders_.count(compile_shader->GetShaderIdentifier()) == 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        compile_shader->GetShaderIdentifierToken(),
        "Identifier '" + compile_shader->GetShaderIdentifier() +
            "' does not correspond to a declared shader");
    return false;
  }
  compiled_shaders_.insert(
      {compile_shader->GetResultIdentifier(), compile_shader});
  return true;
}

bool Checker::VisitCreateBuffer(CommandCreateBuffer* command_create_buffer) {
  if (!CheckIdentifierIsFresh(
          command_create_buffer->GetResultIdentifierToken())) {
    return false;
  }
  return true;
}

bool Checker::VisitCreateSampler(CommandCreateSampler* command_create_sampler) {
  if (!CheckIdentifierIsFresh(
          command_create_sampler->GetResultIdentifierToken())) {
    return false;
  }
  return true;
}

bool Checker::VisitCreateEmptyTexture2D(
    CommandCreateEmptyTexture2D* command_create_empty_texture_2d) {
  if (!CheckIdentifierIsFresh(
          command_create_empty_texture_2d->GetResultIdentifierToken())) {
    return false;
  }
  return true;
}

bool Checker::VisitCreateProgram(CommandCreateProgram* create_program) {
  bool result = true;
  if (!CheckIdentifierIsFresh(create_program->GetResultIdentifierToken())) {
    result = false;
  } else {
    created_programs_.insert(
        {create_program->GetResultIdentifier(), create_program});
  }
  const Token* compiled_vert_shader = nullptr;
  const Token* compiled_frag_shader = nullptr;
  const Token* compiled_comp_shader = nullptr;
  for (size_t index = 0; index < create_program->GetNumCompiledShaders();
       index++) {
    const auto* compiled_shader_identifier =
        create_program->GetCompiledShaderIdentifierToken(index);
    if (compiled_shaders_.count(compiled_shader_identifier->GetText()) == 0) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, compiled_shader_identifier,
          "Identifier '" + compiled_shader_identifier->GetText() +
              "' does not correspond to a compiled shader");
      result = false;
    } else {
      auto shader_kind =
          declared_shaders_
              .at(compiled_shaders_.at(compiled_shader_identifier->GetText())
                      ->GetShaderIdentifier())
              ->GetKind();
      switch (shader_kind) {
        case CommandDeclareShader::Kind::FRAGMENT:
          if (compiled_frag_shader != nullptr) {
            message_consumer_->Message(
                MessageConsumer::Severity::kError, compiled_shader_identifier,
                "Multiple fragment shaders provided to 'CREATE_PROGRAM'; "
                "already found '" +
                    compiled_frag_shader->GetText() + "' at " +
                    compiled_frag_shader->GetLocationString());
            result = false;
          } else {
            compiled_frag_shader = compiled_shader_identifier;
          }
          break;
        case CommandDeclareShader::Kind::VERTEX:
          if (compiled_vert_shader != nullptr) {
            message_consumer_->Message(
                MessageConsumer::Severity::kError, compiled_shader_identifier,
                "Multiple vertex shaders provided to 'CREATE_PROGRAM'; already "
                "found '" +
                    compiled_vert_shader->GetText() + "' at " +
                    compiled_vert_shader->GetLocationString());
            result = false;
          } else {
            compiled_vert_shader = compiled_shader_identifier;
          }
          break;
        case CommandDeclareShader::Kind::COMPUTE:
          if (compiled_comp_shader != nullptr) {
            message_consumer_->Message(
                MessageConsumer::Severity::kError, compiled_shader_identifier,
                "Multiple compute shaders provided to 'CREATE_PROGRAM'; "
                "already "
                "found '" +
                    compiled_comp_shader->GetText() + "' at " +
                    compiled_comp_shader->GetLocationString());
            result = false;
          } else {
            compiled_comp_shader = compiled_shader_identifier;
          }
          break;
      }
    }
  }
  if (compiled_comp_shader != nullptr) {
    if (compiled_frag_shader != nullptr) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, compiled_comp_shader,
          "A compute shader cannot be used in 'CREATE_PROGRAM' with another "
          "kind of shader; found fragment shader '" +
              compiled_frag_shader->GetText() + "' at " +
              compiled_frag_shader->GetLocationString());
      result = false;
    }
    if (compiled_vert_shader != nullptr) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, compiled_comp_shader,
          "A compute shader cannot be used in 'CREATE_PROGRAM' with another "
          "kind of shader; found vertex shader '" +
              compiled_vert_shader->GetText() + "' at " +
              compiled_vert_shader->GetLocationString());
      result = false;
    }
  } else {
    if (compiled_frag_shader == nullptr) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, create_program->GetStartToken(),
          "No fragment shader provided for 'CREATE_PROGRAM' command");
      result = false;
    }
    if (compiled_vert_shader == nullptr) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, create_program->GetStartToken(),
          "No vertex shader provided for 'CREATE_PROGRAM' command");
      result = false;
    }
  }
  return result;
}

bool Checker::VisitCreateRenderbuffer(
    CommandCreateRenderbuffer* command_create_renderbuffer) {
  (void)command_create_renderbuffer;
  return true;
}

bool Checker::VisitDeclareShader(CommandDeclareShader* declare_shader) {
  if (!CheckIdentifierIsFresh(declare_shader->GetResultIdentifierToken())) {
    return false;
  }
  // TODO(afd): Invoke glslang to check that the shader is valid.
  declared_shaders_.insert(
      {declare_shader->GetResultIdentifier(), declare_shader});
  return true;
}

bool Checker::VisitDumpRenderbuffer(
    CommandDumpRenderbuffer* command_dump_renderbuffer) {
  (void)command_dump_renderbuffer;
  return true;
}

bool Checker::VisitRunCompute(CommandRunCompute* command_run_compute) {
  // TODO(afd): Check that the given program is a compute program.
  (void)command_run_compute;
  return true;
}

bool Checker::VisitRunGraphics(CommandRunGraphics* command_run_graphics) {
  // TODO(afd): Check that the given program is a graphics program.
  (void)command_run_graphics;
  return true;
}

bool Checker::VisitSetSamplerOrTextureParameter(
    CommandSetSamplerOrTextureParameter*
        command_set_sampler_or_texture_parameter) {
  (void)command_set_sampler_or_texture_parameter;
  return true;
}

bool Checker::VisitSetUniform(CommandSetUniform* command_set_uniform) {
  (void)command_set_uniform;
  return true;
}

bool Checker::CheckIdentifierIsFresh(const Token* identifier) {
  if (used_identifiers_.count(identifier->GetText()) > 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, identifier,
        "Identifier '" + identifier->GetText() + "' already used at " +
            used_identifiers_.at(identifier->GetText())->GetLocationString());
    return false;
  }
  used_identifiers_.insert({identifier->GetText(), identifier});
  return true;
}

}  // namespace shadertrap
