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

#include <cassert>
#include <cctype>
#include <cstddef>
#include <initializer_list>
#include <type_traits>  // IWYU pragma: keep
#include <utility>
#include <vector>

#include "libshadertrap/make_unique.h"
#include "libshadertrap/tokenizer.h"
#include "libshadertrap/vertex_attribute_info.h"

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

Checker::Checker(MessageConsumer* message_consumer, ApiVersion api_version)
    : message_consumer_(message_consumer), api_version_(api_version) {}

bool Checker::VisitAssertEqual(CommandAssertEqual* command_assert_equal) {
  const auto& operand1_token =
      command_assert_equal->GetArgumentIdentifier1Token();
  const auto& operand2_token =
      command_assert_equal->GetArgumentIdentifier2Token();
  bool found_errors = false;
  if (command_assert_equal->GetArgumentsAreRenderbuffers()) {
    for (const auto& operand_token : {operand1_token, operand2_token}) {
      if (created_renderbuffers_.count(operand_token.GetText()) == 0) {
        message_consumer_->Message(
            MessageConsumer::Severity::kError, &operand_token,
            "'" + operand_token.GetText() + "' must be a renderbuffer");
        found_errors = true;
      }
    }
    if (found_errors) {
      return false;
    }
    if (!CheckRenderbufferDimensionsMatch(operand1_token, operand2_token)) {
      return false;
    }
  } else {
    for (const auto& operand_token : {operand1_token, operand2_token}) {
      if (created_buffers_.count(operand_token.GetText()) == 0) {
        message_consumer_->Message(
            MessageConsumer::Severity::kError, &operand_token,
            "'" + operand_token.GetText() + "' must be a buffer");
        found_errors = true;
      }
    }
    if (found_errors) {
      return false;
    }
    auto* buffer1 = created_buffers_.at(operand1_token.GetText());
    auto* buffer2 = created_buffers_.at(operand2_token.GetText());
    if (buffer1->GetSizeBytes() != buffer2->GetSizeBytes()) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, &operand2_token,
          "size (in bytes) " + std::to_string(buffer2->GetSizeBytes()) +
              " of '" + operand2_token.GetText() +
              "' does not match size (in bytes) " +
              std::to_string(buffer1->GetSizeBytes()) + " of '" +
              operand1_token.GetText() + "' at " +
              operand1_token.GetLocationString());
      found_errors = true;
    }
  }
  auto& format_entries = command_assert_equal->GetFormatEntries();
  if (!format_entries.empty()) {
    size_t total_count_bytes = 0;
    for (const auto& format_entry : format_entries) {
      if (format_entry.count == 0) {
        message_consumer_->Message(
            MessageConsumer::Severity::kError, format_entry.token.get(),
            "The count for a formatting entry must be positive");
        found_errors = true;
      }
      switch (format_entry.kind) {
        case CommandAssertEqual::FormatEntry::Kind::kByte:
        case CommandAssertEqual::FormatEntry::Kind::kSkip:
          if (format_entry.count % 4 != 0) {
            message_consumer_->Message(
                MessageConsumer::Severity::kError, format_entry.token.get(),
                "The count for a '" +
                    Tokenizer::KeywordToString(
                        format_entry.kind ==
                                CommandAssertEqual::FormatEntry::Kind::kByte
                            ? Token::Type::kKeywordTypeByte
                            : Token::Type::kKeywordSkipBytes) +
                    "' formatting entry must be a multiple of 4; found " +
                    std::to_string(format_entry.count));
            found_errors = true;
          }
          total_count_bytes += format_entry.count;
          break;
        case CommandAssertEqual::FormatEntry::Kind::kFloat:
        case CommandAssertEqual::FormatEntry::Kind::kInt:
        case CommandAssertEqual::FormatEntry::Kind::kUint:
          total_count_bytes += format_entry.count * 4;
          break;
      }
    }

    auto* buffer1 = created_buffers_.at(operand1_token.GetText());
    auto* buffer2 = created_buffers_.at(operand2_token.GetText());
    const size_t expected_bytes = buffer1->GetSizeBytes();

    if (total_count_bytes != expected_bytes) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError,
          command_assert_equal->GetFormatEntries()[0].token.get(),
          "The number of bytes specified in the formatting of '" +
              buffer1->GetResultIdentifier() + "(" +
              buffer2->GetResultIdentifier() + ")" + "' is " +
              std::to_string(total_count_bytes) + ", but '" +
              buffer1->GetResultIdentifier() + "(" +
              buffer2->GetResultIdentifier() + ")" +
              "' was declared with size " + std::to_string(expected_bytes) +
              " byte" + (expected_bytes > 1 ? "s" : "") + " at " +
              buffer1->GetStartToken().GetLocationString());
      found_errors = true;
    }
  }
  return !found_errors;
}

bool Checker::VisitAssertPixels(CommandAssertPixels* command_assert_pixels) {
  if (created_renderbuffers_.count(
          command_assert_pixels->GetRenderbufferIdentifier()) == 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_assert_pixels->GetRenderbufferIdentifierToken(),
        "'" + command_assert_pixels->GetRenderbufferIdentifier() +
            "' is not a renderbuffer");
    return false;
  }
  bool found_errors = false;
  if (command_assert_pixels->GetRectangleWidth() == 0) {
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               &command_assert_pixels->GetRectangleWidthToken(),
                               "width of rectangle must be positive");
    found_errors = true;
  }
  if (command_assert_pixels->GetRectangleHeight() == 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_assert_pixels->GetRectangleHeightToken(),
        "height of rectangle must be positive");
    found_errors = true;
  }
  const auto* renderbuffer = created_renderbuffers_.at(
      command_assert_pixels->GetRenderbufferIdentifier());
  size_t width_plus_x = command_assert_pixels->GetRectangleWidth() +
                        command_assert_pixels->GetRectangleX();
  if (width_plus_x > renderbuffer->GetWidth()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_assert_pixels->GetRectangleWidthToken(),
        "rectangle extends to x-coordinate " + std::to_string(width_plus_x) +
            ", which exceeds width " +
            std::to_string(renderbuffer->GetWidth()) + " of '" +
            command_assert_pixels->GetRenderbufferIdentifier() + "'");
    found_errors = true;
  }
  size_t height_plus_y = command_assert_pixels->GetRectangleHeight() +
                         command_assert_pixels->GetRectangleY();
  if (height_plus_y > renderbuffer->GetHeight()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_assert_pixels->GetRectangleHeightToken(),
        "rectangle extends to y-coordinate " + std::to_string(height_plus_y) +
            ", which exceeds height " +
            std::to_string(renderbuffer->GetHeight()) + " of '" +
            command_assert_pixels->GetRenderbufferIdentifier() + "'");
    found_errors = true;
  }
  return !found_errors;
}

bool Checker::VisitAssertSimilarEmdHistogram(
    CommandAssertSimilarEmdHistogram* command_assert_similar_emd_histogram) {
  bool both_renderbuffers_present = true;
  if (created_renderbuffers_.count(
          command_assert_similar_emd_histogram->GetRenderbufferIdentifier1()) ==
      0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_assert_similar_emd_histogram
             ->GetRenderbufferIdentifier1Token(),
        "'" +
            command_assert_similar_emd_histogram->GetRenderbufferIdentifier1() +
            "' must be a renderbuffer");
    both_renderbuffers_present = false;
  }
  if (created_renderbuffers_.count(
          command_assert_similar_emd_histogram->GetRenderbufferIdentifier2()) ==
      0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_assert_similar_emd_histogram
             ->GetRenderbufferIdentifier2Token(),
        "'" +
            command_assert_similar_emd_histogram->GetRenderbufferIdentifier2() +
            "' must be a renderbuffer");
    both_renderbuffers_present = false;
  }
  if (!both_renderbuffers_present) {
    return false;
  }
  return CheckRenderbufferDimensionsMatch(
      command_assert_similar_emd_histogram->GetRenderbufferIdentifier1Token(),
      command_assert_similar_emd_histogram->GetRenderbufferIdentifier2Token());
}

bool Checker::VisitBindSampler(CommandBindSampler* command_bind_sampler) {
  if (created_samplers_.count(command_bind_sampler->GetSamplerIdentifier()) ==
      0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_bind_sampler->GetSamplerIdentifierToken(),
        "'" + command_bind_sampler->GetSamplerIdentifier() +
            "' must be a sampler");
    return false;
  }
  return true;
}

bool Checker::VisitBindShaderStorageBuffer(
    CommandBindShaderStorageBuffer* command_bind_shader_storage_buffer) {
  if (created_buffers_.count(
          command_bind_shader_storage_buffer->GetBufferIdentifier()) == 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_bind_shader_storage_buffer->GetBufferIdentifierToken(),
        "'" + command_bind_shader_storage_buffer->GetBufferIdentifier() +
            "' must be a buffer");
    return false;
  }
  return true;
}

bool Checker::VisitBindTexture(CommandBindTexture* command_bind_texture) {
  if (created_textures_.count(command_bind_texture->GetTextureIdentifier()) ==
      0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_bind_texture->GetTextureIdentifierToken(),
        "'" + command_bind_texture->GetTextureIdentifier() +
            "' must be a texture");
    return false;
  }
  return true;
}

bool Checker::VisitBindUniformBuffer(
    CommandBindUniformBuffer* command_bind_uniform_buffer) {
  if (created_buffers_.count(
          command_bind_uniform_buffer->GetBufferIdentifier()) == 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_bind_uniform_buffer->GetBufferIdentifierToken(),
        "'" + command_bind_uniform_buffer->GetBufferIdentifier() +
            "' must be a buffer");
    return false;
  }
  return true;
}

bool Checker::VisitCompileShader(CommandCompileShader* compile_shader) {
  if (!CheckIdentifierIsFresh(compile_shader->GetResultIdentifierToken())) {
    return false;
  }
  if (declared_shaders_.count(compile_shader->GetShaderIdentifier()) == 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &compile_shader->GetShaderIdentifierToken(),
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
  created_buffers_.insert(
      {command_create_buffer->GetResultIdentifier(), command_create_buffer});
  return true;
}

bool Checker::VisitCreateSampler(CommandCreateSampler* command_create_sampler) {
  if (!CheckIdentifierIsFresh(
          command_create_sampler->GetResultIdentifierToken())) {
    return false;
  }
  created_samplers_.insert(
      {command_create_sampler->GetResultIdentifier(), command_create_sampler});
  return true;
}

bool Checker::VisitCreateEmptyTexture2D(
    CommandCreateEmptyTexture2D* command_create_empty_texture_2d) {
  if (!CheckIdentifierIsFresh(
          command_create_empty_texture_2d->GetResultIdentifierToken())) {
    return false;
  }
  created_textures_.insert(
      {command_create_empty_texture_2d->GetResultIdentifier(),
       command_create_empty_texture_2d});
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
    const auto& compiled_shader_identifier =
        create_program->GetCompiledShaderIdentifierToken(index);
    if (compiled_shaders_.count(compiled_shader_identifier.GetText()) == 0) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, &compiled_shader_identifier,
          "Identifier '" + compiled_shader_identifier.GetText() +
              "' does not correspond to a compiled shader");
      result = false;
    } else {
      auto shader_kind =
          declared_shaders_
              .at(compiled_shaders_.at(compiled_shader_identifier.GetText())
                      ->GetShaderIdentifier())
              ->GetKind();
      switch (shader_kind) {
        case CommandDeclareShader::Kind::FRAGMENT:
          if (compiled_frag_shader != nullptr) {
            message_consumer_->Message(
                MessageConsumer::Severity::kError, &compiled_shader_identifier,
                "Multiple fragment shaders provided to 'CREATE_PROGRAM'; "
                "already found '" +
                    compiled_frag_shader->GetText() + "' at " +
                    compiled_frag_shader->GetLocationString());
            result = false;
          } else {
            compiled_frag_shader = &compiled_shader_identifier;
          }
          break;
        case CommandDeclareShader::Kind::VERTEX:
          if (compiled_vert_shader != nullptr) {
            message_consumer_->Message(
                MessageConsumer::Severity::kError, &compiled_shader_identifier,
                "Multiple vertex shaders provided to 'CREATE_PROGRAM'; already "
                "found '" +
                    compiled_vert_shader->GetText() + "' at " +
                    compiled_vert_shader->GetLocationString());
            result = false;
          } else {
            compiled_vert_shader = &compiled_shader_identifier;
          }
          break;
        case CommandDeclareShader::Kind::COMPUTE:
          if (compiled_comp_shader != nullptr) {
            message_consumer_->Message(
                MessageConsumer::Severity::kError, &compiled_shader_identifier,
                "Multiple compute shaders provided to 'CREATE_PROGRAM'; "
                "already "
                "found '" +
                    compiled_comp_shader->GetText() + "' at " +
                    compiled_comp_shader->GetLocationString());
            result = false;
          } else {
            compiled_comp_shader = &compiled_shader_identifier;
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
          MessageConsumer::Severity::kError, &create_program->GetStartToken(),
          "No fragment shader provided for 'CREATE_PROGRAM' command");
      result = false;
    }
    if (compiled_vert_shader == nullptr) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, &create_program->GetStartToken(),
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
        MessageConsumer::Severity::kError, &create_program->GetStartToken(),
        "Linking of program '" + create_program->GetResultIdentifier() +
            "' using glslang failed. Line numbers in the following output are "
            "offsets from the start of the provided shader text string:\n" +
            std::string(glslang_program->getInfoLog()));
    return false;
  }
  if (!glslang_program->buildReflection()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, &create_program->GetStartToken(),
        "Building reflection data for program '" +
            create_program->GetResultIdentifier() +
            "' using glslang failed. Line numbers in the following output are "
            "offsets from the start of the provided shader text string:\n" +
            std::string(glslang_program->getInfoLog()));
    return false;
  }
  glslang_programs_.insert(
      {create_program->GetResultIdentifier(), std::move(glslang_program)});
  return true;
}

bool Checker::VisitCreateRenderbuffer(
    CommandCreateRenderbuffer* command_create_renderbuffer) {
  if (!CheckIdentifierIsFresh(
          command_create_renderbuffer->GetResultIdentifierToken())) {
    return false;
  }
  created_renderbuffers_.insert(
      {command_create_renderbuffer->GetResultIdentifier(),
       command_create_renderbuffer});
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
      if ((api_version_.GetApi() == ApiVersion::Api::GL &&
           api_version_ < ApiVersion(ApiVersion::Api::GL, 4, 3)) ||
          (api_version_.GetApi() == ApiVersion::Api::GLES &&
           api_version_ < ApiVersion(ApiVersion::Api::GLES, 3, 1))) {
        message_consumer_->Message(MessageConsumer::Severity::kError,
                                   &declare_shader->GetStartToken(),
                                   "Compute shaders are not supported before "
                                   "OpenGL 4.3 or OpenGL ES 3.1");
        return false;
      }
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
        MessageConsumer::Severity::kError, &declare_shader->GetStartToken(),
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
  if (created_renderbuffers_.count(
          command_dump_renderbuffer->GetRenderbufferIdentifier()) == 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_dump_renderbuffer->GetRenderbufferIdentifierToken(),
        "'" + command_dump_renderbuffer->GetRenderbufferIdentifier() +
            "' must be a renderbuffer");
    return false;
  }
  return true;
}

bool Checker::VisitDumpBufferBinary(
    CommandDumpBufferBinary* dump_buffer_binary) {
  if (created_buffers_.count(dump_buffer_binary->GetBufferIdentifier()) == 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &dump_buffer_binary->GetBufferIdentifierToken(),
        "'" + dump_buffer_binary->GetBufferIdentifier() + "' must be a buffer");
    return false;
  }
  return true;
}

bool Checker::VisitDumpBufferText(CommandDumpBufferText* dump_buffer_text) {
  if (created_buffers_.count(dump_buffer_text->GetBufferIdentifier()) == 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &dump_buffer_text->GetBufferIdentifierToken(),
        "'" + dump_buffer_text->GetBufferIdentifier() + "' must be a buffer");
    return false;
  }
  bool errors_found = false;
  size_t total_count_bytes = 0;
  for (const auto& format_entry : dump_buffer_text->GetFormatEntries()) {
    switch (format_entry.kind) {
      case CommandDumpBufferText::FormatEntry::Kind::kString:
        break;
      case CommandDumpBufferText::FormatEntry::Kind::kByte:
      case CommandDumpBufferText::FormatEntry::Kind::kSkip:
        if (format_entry.count == 0) {
          message_consumer_->Message(
              MessageConsumer::Severity::kError, format_entry.token.get(),
              "The count for a formatting entry must be positive");
          errors_found = true;
        }
        if (format_entry.count % 4 != 0) {
          message_consumer_->Message(
              MessageConsumer::Severity::kError, format_entry.token.get(),
              "The count for a '" +
                  Tokenizer::KeywordToString(
                      format_entry.kind ==
                              CommandDumpBufferText::FormatEntry::Kind::kByte
                          ? Token::Type::kKeywordTypeByte
                          : Token::Type::kKeywordSkipBytes) +
                  "' formatting entry must be a multiple of 4; found " +
                  std::to_string(format_entry.count));
          errors_found = true;
        }
        total_count_bytes += format_entry.count;
        break;
      case CommandDumpBufferText::FormatEntry::Kind::kFloat:
      case CommandDumpBufferText::FormatEntry::Kind::kInt:
      case CommandDumpBufferText::FormatEntry::Kind::kUint:
        if (format_entry.count == 0) {
          message_consumer_->Message(
              MessageConsumer::Severity::kError, format_entry.token.get(),
              "The count for a formatting entry must be positive");
          errors_found = true;
        }
        total_count_bytes += format_entry.count * 4;
        break;
    }
  }
  auto* buffer = created_buffers_.at(dump_buffer_text->GetBufferIdentifier());
  const size_t expected_bytes = buffer->GetSizeBytes();
  if (total_count_bytes != expected_bytes) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        dump_buffer_text->GetFormatEntries()[0].token.get(),
        "The number of bytes specified in the formatting of '" +
            buffer->GetResultIdentifier() + "' is " +
            std::to_string(total_count_bytes) + ", but '" +
            buffer->GetResultIdentifier() + "' was declared with size " +
            std::to_string(expected_bytes) + " byte" +
            (expected_bytes > 1 ? "s" : "") + " at " +
            buffer->GetStartToken().GetLocationString());
    errors_found = true;
  }
  return !errors_found;
}

bool Checker::VisitRunCompute(CommandRunCompute* command_run_compute) {
  if (created_programs_.count(command_run_compute->GetProgramIdentifier()) ==
      0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_run_compute->GetProgramIdentifierToken(),
        "'" + command_run_compute->GetProgramIdentifier() +
            "' must be a program");
    return false;
  }
  if (created_programs_.at(command_run_compute->GetProgramIdentifier())
          ->GetNumCompiledShaders() != 1) {
    // A compute program comprises a single (compute) shader; if there is not
    // exactly one shader then this must be a graphics program.
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_run_compute->GetProgramIdentifierToken(),
        "'" + command_run_compute->GetProgramIdentifier() +
            "' must be a compute program, not a graphics program");
    return false;
  }
  return true;
}

bool Checker::VisitRunGraphics(CommandRunGraphics* command_run_graphics) {
  bool errors_found = false;
  if (created_programs_.count(command_run_graphics->GetProgramIdentifier()) ==
      0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_run_graphics->GetProgramIdentifierToken(),
        "'" + command_run_graphics->GetProgramIdentifier() +
            "' must be a program");
    errors_found = true;
  } else if (created_programs_.at(command_run_graphics->GetProgramIdentifier())
                 ->GetNumCompiledShaders() != 2) {
    // A graphics program comprises a pair of (vertex and fragment) shaders; if
    // there is not exactly two shaders then this must be a compute program.
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_run_graphics->GetProgramIdentifierToken(),
        "'" + command_run_graphics->GetProgramIdentifier() +
            "' must be a graphics program, not a compute program");
    errors_found = true;
  }
  for (const auto& entry : command_run_graphics->GetVertexData()) {
    if (created_buffers_.count(entry.second.GetBufferIdentifier()) == 0) {
      message_consumer_->Message(MessageConsumer::Severity::kError,
                                 &entry.second.GetBufferIdentifierToken(),
                                 "vertex buffer '" +
                                     entry.second.GetBufferIdentifier() +
                                     "' must be a buffer");
      errors_found = true;
    }
  }
  if (created_buffers_.count(
          command_run_graphics->GetIndexDataBufferIdentifier()) == 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_run_graphics->GetIndexDataBufferIdentifierToken(),
        "index buffer '" +
            command_run_graphics->GetIndexDataBufferIdentifier() +
            "' must be a buffer");
    errors_found = true;
  }
  for (const auto& entry : command_run_graphics->GetFramebufferAttachments()) {
    if (api_version_ == ApiVersion(ApiVersion::Api::GLES, 2, 0) &&
        entry.first != 0) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, entry.second.get(),
          "Only 0 may be used as a framebuffer attachment key when working "
          "with OpenGL ES 2.0");
      errors_found = true;
    }
    if (created_renderbuffers_.count(entry.second->GetText()) == 0 &&
        created_textures_.count(entry.second->GetText()) == 0) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, entry.second.get(),
          "framebuffer attachment '" + entry.second->GetText() +
              "' must be a renderbuffer or texture");
      errors_found = true;
    }
  }
  return !errors_found;
}

bool Checker::VisitSetSamplerParameter(
    CommandSetSamplerParameter* command_set_sampler_parameter) {
  if (created_samplers_.count(
          command_set_sampler_parameter->GetSamplerIdentifier()) == 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_set_sampler_parameter->GetSamplerIdentifierToken(),
        "'" + command_set_sampler_parameter->GetSamplerIdentifier() +
            "' must be a sampler");
    return false;
  }
  return true;
}

bool Checker::VisitSetTextureParameter(
    CommandSetTextureParameter* command_set_texture_parameter) {
  if (created_textures_.count(
          command_set_texture_parameter->GetTextureIdentifier()) == 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_set_texture_parameter->GetTextureIdentifierToken(),
        "'" + command_set_texture_parameter->GetTextureIdentifier() +
            "' must be a texture");
    return false;
  }
  return true;
}

bool Checker::VisitSetUniform(CommandSetUniform* command_set_uniform) {
  if (created_programs_.count(command_set_uniform->GetProgramIdentifier()) ==
      0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &command_set_uniform->GetProgramIdentifierToken(),
        "'" + command_set_uniform->GetProgramIdentifier() +
            "' must be a program");
    return false;
  }
  // TODO(afd): The uniform index must be valid.
  return true;
}

bool Checker::CheckIdentifierIsFresh(const Token& identifier) {
  if (used_identifiers_.count(identifier.GetText()) > 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, &identifier,
        "Identifier '" + identifier.GetText() + "' already used at " +
            used_identifiers_.at(identifier.GetText()).GetLocationString());
    return false;
  }
  used_identifiers_.insert({identifier.GetText(), identifier});
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

bool Checker::CheckRenderbufferDimensionsMatch(
    const Token& renderbuffer_token_1, const Token& renderbuffer_token_2) {
  assert(created_renderbuffers_.count(renderbuffer_token_1.GetText()) != 0 &&
         "First argument must be a renderbuffer.");
  assert(created_renderbuffers_.count(renderbuffer_token_2.GetText()) != 0 &&
         "Second argument must be a renderbuffer.");
  bool result = true;
  auto* renderbuffer1 =
      created_renderbuffers_.at(renderbuffer_token_1.GetText());
  auto* renderbuffer2 =
      created_renderbuffers_.at(renderbuffer_token_2.GetText());
  if (renderbuffer1->GetWidth() != renderbuffer2->GetWidth()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, &renderbuffer_token_2,
        "width " + std::to_string(renderbuffer2->GetWidth()) + " of '" +
            renderbuffer_token_2.GetText() + "' does not match width " +
            std::to_string(renderbuffer1->GetWidth()) + " of '" +
            renderbuffer_token_1.GetText() + "' at " +
            renderbuffer_token_1.GetLocationString());
    result = false;
  }
  if (renderbuffer1->GetHeight() != renderbuffer2->GetHeight()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, &renderbuffer_token_2,
        "height " + std::to_string(renderbuffer2->GetHeight()) + " of '" +
            renderbuffer_token_2.GetText() + "' does not match height " +
            std::to_string(renderbuffer1->GetHeight()) + " of '" +
            renderbuffer_token_1.GetText() + "' at " +
            renderbuffer_token_1.GetLocationString());
    result = false;
  }
  return result;
}

}  // namespace shadertrap
