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

#include <cctype>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>

#include "libshadertrap/make_unique.h"

namespace shadertrap {

namespace {

const TBuiltInResource kDefaultTBuiltInResource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,
    /* .maxDualSourceDrawBuffersEXT = */ 1,

    /* .limits = */
    {
        /* .nonInductiveForLoops = */ true,
        /* .whileLoops = */ true,
        /* .doWhileLoops = */ true,
        /* .generalUniformIndexing = */ true,
        /* .generalAttributeMatrixVectorIndexing = */ true,
        /* .generalVaryingIndexing = */ true,
        /* .generalSamplerIndexing = */ true,
        /* .generalVariableIndexing = */ true,
        /* .generalConstantMatrixVectorIndexing = */ true,
    }};

}  // namespace

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
  // TODO(afd): Both arguments must be renderbuffers
  // TODO(afd): Both arguments must have the same dimensions
  (void)command_assert_similar_emd_histogram;
  return true;
}

bool Checker::VisitBindSampler(CommandBindSampler* command_bind_sampler) {
  // TODO(afd): Check that the given sampler exists.
  (void)command_bind_sampler;
  return true;
}

bool Checker::VisitBindStorageBuffer(
    CommandBindStorageBuffer* command_bind_storage_buffer) {
  // TODO(afd): Check that the given buffer exists.
  (void)command_bind_storage_buffer;
  return true;
}

bool Checker::VisitBindTexture(CommandBindTexture* command_bind_texture) {
  // TODO(afd): Check that the given texture exists.
  (void)command_bind_texture;
  return true;
}

bool Checker::VisitBindUniformBuffer(
    CommandBindUniformBuffer* command_bind_uniform_buffer) {
  // TODO(afd): Check that the given buffer exists.
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
  if (!result) {
    return false;
  }

  auto glslang_program = MakeUnique<glslang::TProgram>();
  for (size_t i = 0; i < create_program->GetNumCompiledShaders(); i++) {
    CommandCompileShader* compile_shader_command =
        compiled_shaders_.at(create_program->GetCompiledShaderIdentifier(i));
    glslang_program->addShader(
        glslang_shaders_.at(compile_shader_command->GetShaderIdentifier())
            .get());
  }
  if (!glslang_program->link(EShMsgDefault)) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, create_program->GetStartToken(),
        "Linking of program '" + create_program->GetResultIdentifier() +
            "' using glslang failed. Line numbers in the following output are "
            "offsets from the start of the provided shader text string:\n" +
            std::string(glslang_program->getInfoLog()));
  }
  return true;
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
  EShLanguage shader_stage = EShLanguage::EShLangVertex;
  switch (declare_shader->GetKind()) {
    case CommandDeclareShader::Kind::VERTEX:
      shader_stage = EShLanguage::EShLangVertex;
      break;
    case CommandDeclareShader::Kind::FRAGMENT:
      shader_stage = EShLanguage::EShLangFragment;
      break;
    case CommandDeclareShader::Kind::COMPUTE:
      shader_stage = EShLanguage::EShLangCompute;
      break;
  }
  auto glslang_shader = MakeUnique<glslang::TShader>(shader_stage);
  const auto* shader_text = declare_shader->GetShaderText().c_str();
  const int shader_text_length =
      static_cast<int>(declare_shader->GetShaderText().size());
  glslang_shader->setStringsWithLengths(&shader_text, &shader_text_length, 1);
  const int kGlslVersion100 = 100;
  if (!glslang_shader->parse(&kDefaultTBuiltInResource, kGlslVersion100, false,
                             EShMsgDefault)) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, declare_shader->GetStartToken(),
        "Validation of shader '" + declare_shader->GetResultIdentifier() +
            "' using glslang failed with the following messages:\n" +
            FixLinesInGlslangOutput(std::string(glslang_shader->getInfoLog()),
                                    declare_shader->GetShaderStartLine() - 1));
    return false;
  }
  declared_shaders_.insert(
      {declare_shader->GetResultIdentifier(), declare_shader});
  glslang_shaders_.insert(
      {declare_shader->GetResultIdentifier(), std::move(glslang_shader)});
  return true;
}

bool Checker::VisitDumpRenderbuffer(
    CommandDumpRenderbuffer* command_dump_renderbuffer) {
  // TODO(afd): The given buffer must be a renderbuffer.
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
  // TODO(afd): The sampler or texture must exist.
  (void)command_set_sampler_or_texture_parameter;
  return true;
}

bool Checker::VisitSetUniform(CommandSetUniform* command_set_uniform) {
  // TODO(afd): The program must exist. The uniform index must be valid.
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

std::string Checker::FixLinesInGlslangOutput(const std::string& glslang_output,
                                             size_t line_offset) {
  std::string result;
  size_t pos = 0;
  bool is_line_start = true;
  // Look through all of the glslang output
  while (pos < glslang_output.size()) {
    if (!is_line_start) {
      // We are not at the start of a line, so we should pass this character
      // through to |result|. However, if it's an end of line character we
      // note that we now are at the start of a line.
      if (glslang_output[pos] == '\n') {
        is_line_start = true;
      }
      result.push_back(glslang_output[pos]);
      pos++;
    } else {
      // We are at the start of a line, so we see whether this line starts with
      // a glslang message.
      // We first note that we're no longer at the start of a line. If the line
      // happens to be empty -- i.e., to just contain a '\n' character -- that's
      // OK; we will process that character on the next loop iteration.
      is_line_start = false;
      bool found_message_prefix = false;
      // Check whether the line starts with one of the known message prefixes.
      for (std::string prefix :
           {"WARNING: ", "ERROR: ", "INTERNAL ERROR: ", "UNIMPLEMENTED: ",
            "NOTE: ", "UNKNOWN ERROR: "}) {
        if (glslang_output.substr(pos, std::string(prefix).length()) ==
            prefix) {
          result.append(prefix);
          pos += prefix.length();
          found_message_prefix = true;
          break;
        }
      }
      if (found_message_prefix) {
        // The line starts with a known prefix, so we need to check whether it's
        // followed by text matching the pattern "\d+:(\d+)", which we want to
        // replace with "line $1".
        if (pos < glslang_output.length() &&
            std::isdigit(glslang_output[pos]) != 0) {
          // If we do find that the upcoming characters match the patern of
          // interest then we want to ignore the first group of digits and the
          // colon, but if the pattern is only partially matched then we should
          // output them, so we save them up in a temporary.
          std::string characters_to_ignore_if_pattern_is_matched;
          bool pattern_is_matched = false;
          while (pos < glslang_output.length() &&
                 std::isdigit(glslang_output[pos]) != 0) {
            characters_to_ignore_if_pattern_is_matched.push_back(
                glslang_output[pos]);
            pos++;
          }
          if (pos < glslang_output.length() && glslang_output[pos] == ':') {
            characters_to_ignore_if_pattern_is_matched.push_back(
                glslang_output[pos]);
            pos++;
            if (pos < glslang_output.length() &&
                std::isdigit(glslang_output[pos]) != 0) {
              std::string line_digits;
              while (pos < glslang_output.length() &&
                     std::isdigit(glslang_output[pos]) != 0) {
                line_digits.push_back(glslang_output[pos]);
                pos++;
              }
              pattern_is_matched = true;
              result.append(
                  "line " +
                  std::to_string(static_cast<size_t>(std::stoi(line_digits)) +
                                 line_offset));
            }
          }
          if (!pattern_is_matched) {
            result.append(characters_to_ignore_if_pattern_is_matched);
          }
        }
      }
    }
  }
  return result;
}

}  // namespace shadertrap
