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

#include "libshadertrap/parser.h"

#include <cassert>
#include <set>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "libshadertrap/command_assert_equal.h"
#include "libshadertrap/command_assert_pixels.h"
#include "libshadertrap/command_assert_similar_emd_histogram.h"
#include "libshadertrap/command_bind_sampler.h"
#include "libshadertrap/command_bind_storage_buffer.h"
#include "libshadertrap/command_bind_texture.h"
#include "libshadertrap/command_bind_uniform_buffer.h"
#include "libshadertrap/command_compile_shader.h"
#include "libshadertrap/command_create_buffer.h"
#include "libshadertrap/command_create_empty_texture_2d.h"
#include "libshadertrap/command_create_program.h"
#include "libshadertrap/command_create_renderbuffer.h"
#include "libshadertrap/command_create_sampler.h"
#include "libshadertrap/command_declare_shader.h"
#include "libshadertrap/command_dump_renderbuffer.h"
#include "libshadertrap/command_run_compute.h"
#include "libshadertrap/command_run_graphics.h"
#include "libshadertrap/command_set_sampler_or_texture_parameter.h"
#include "libshadertrap/command_set_uniform.h"
#include "libshadertrap/make_unique.h"
#include "libshadertrap/token.h"
#include "libshadertrap/tokenizer.h"

namespace shadertrap {

Parser::Parser(const std::string& input, MessageConsumer* message_consumer)
    : tokenizer_(MakeUnique<Tokenizer>(input)),
      message_consumer_(message_consumer) {}

Parser::~Parser() = default;

bool Parser::Parse() {
  while (!tokenizer_->PeekNextToken()->IsEOS()) {
    if (!ParseCommand()) {
      return false;
    }
  }
  return true;
}

bool Parser::ParseCommand() {
  auto token = tokenizer_->PeekNextToken();
  switch (token->GetType()) {
    case Token::Type::kKeywordAssertEqual:
      return ParseCommandAssertEqual();
    case Token::Type::kKeywordAssertPixels:
      return ParseCommandAssertPixels();
    case Token::Type::kKeywordAssertSimilarEmdHistogram:
      return ParseCommandAssertSimilarEmdHistogram();
    case Token::Type::kKeywordBindSampler:
      return ParseCommandBindSampler();
    case Token::Type::kKeywordBindStorageBuffer:
      return ParseCommandBindStorageBuffer();
    case Token::Type::kKeywordBindTexture:
      return ParseCommandBindTexture();
    case Token::Type::kKeywordBindUniformBuffer:
      return ParseCommandBindUniformBuffer();
    case Token::Type::kKeywordCompileShader:
      return ParseCommandCompileShader();
    case Token::Type::kKeywordCreateBuffer:
      return ParseCommandCreateBuffer();
    case Token::Type::kKeywordCreateEmptyTexture2d:
      return ParseCommandCreateEmptyTexture2d();
    case Token::Type::kKeywordCreateProgram:
      return ParseCommandCreateProgram();
    case Token::Type::kKeywordCreateSampler:
      return ParseCommandCreateSampler();
    case Token::Type::kKeywordCreateRenderbuffer:
      return ParseCommandCreateRenderbuffer();
    case Token::Type::kKeywordDeclareShader:
      return ParseCommandDeclareShader();
    case Token::Type::kKeywordDumpRenderbuffer:
      return ParseCommandDumpRenderbuffer();
    case Token::Type::kKeywordRunCompute:
      return ParseCommandRunCompute();
    case Token::Type::kKeywordRunGraphics:
      return ParseCommandRunGraphics();
    case Token::Type::kKeywordSetSamplerParameter:
    case Token::Type::kKeywordSetTextureParameter:
      return ParseCommandSetTextureOrSamplerParameter();
    case Token::Type::kKeywordSetUniform:
      return ParseCommandSetUniform();
    default:
      message_consumer_->Message(MessageConsumer::Severity::kError, token.get(),
                                 "Unknown command: '" + token->GetText() + "'");
      return false;
  }
}

bool Parser::ParseCommandAssertEqual() {
  auto start_token = tokenizer_->NextToken();
  std::unique_ptr<Token> buffer_identifier_1;
  std::unique_ptr<Token> buffer_identifier_2;
  if (!ParseParameters({{Token::Type::kKeywordBuffer1,
                         [this, &buffer_identifier_1]() -> bool {
                           auto token = tokenizer_->NextToken();
                           if (!token->IsIdentifier()) {
                             message_consumer_->Message(
                                 MessageConsumer::Severity::kError, token.get(),
                                 "Expected identifier for first renderbuffer "
                                 "to be compared");
                           }
                           buffer_identifier_1 = std::move(token);
                           return true;
                         }},
                        {Token::Type::kKeywordBuffer2,
                         [this, &buffer_identifier_2]() -> bool {
                           auto token = tokenizer_->NextToken();
                           if (!token->IsIdentifier()) {
                             message_consumer_->Message(
                                 MessageConsumer::Severity::kError, token.get(),
                                 "Expected identifier for second renderbuffer "
                                 "to be compared");
                           }
                           buffer_identifier_2 = std::move(token);
                           return true;
                         }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandAssertEqual>(
      std::move(start_token), std::move(buffer_identifier_1),
      std::move(buffer_identifier_2)));
  return true;
}

bool Parser::ParseCommandAssertPixels() {
  auto start_token = tokenizer_->NextToken();
  uint8_t expected_r;
  uint8_t expected_g;
  uint8_t expected_b;
  uint8_t expected_a;
  std::string renderbuffer_identifier;
  size_t rectangle_x;
  size_t rectangle_y;
  size_t rectangle_width;
  size_t rectangle_height;
  if (!ParseParameters({{Token::Type::kKeywordExpected,
                         [this, &expected_r, &expected_g, &expected_b,
                          &expected_a]() -> bool {
                           auto maybe_expected_r = ParseUint8("r component");
                           if (!maybe_expected_r.first) {
                             return false;
                           }
                           expected_r = maybe_expected_r.second;
                           auto maybe_expected_g = ParseUint8("g component");
                           if (!maybe_expected_g.first) {
                             return false;
                           }
                           expected_g = maybe_expected_g.second;
                           auto maybe_expected_b = ParseUint8("b component");
                           if (!maybe_expected_b.first) {
                             return false;
                           }
                           expected_b = maybe_expected_b.second;
                           auto maybe_expected_a = ParseUint8("a component");
                           if (!maybe_expected_a.first) {
                             return false;
                           }
                           expected_a = maybe_expected_a.second;
                           return true;
                         }},
                        {Token::Type::kKeywordRenderbuffer,
                         [this, &renderbuffer_identifier]() -> bool {
                           auto token = tokenizer_->NextToken();
                           if (!token->IsIdentifier()) {
                             message_consumer_->Message(
                                 MessageConsumer::Severity::kError, token.get(),
                                 "Expected renderbuffer identifier");
                             return false;
                           }
                           renderbuffer_identifier = token->GetText();
                           return true;
                         }},
                        {Token::Type::kKeywordRectangle,
                         [this, &rectangle_x, &rectangle_y, &rectangle_width,
                          &rectangle_height]() -> bool {
                           auto maybe_x = ParseUint32("x coordinate");
                           if (!maybe_x.first) {
                             return false;
                           }
                           rectangle_x = maybe_x.second;
                           auto maybe_y = ParseUint32("y coordinate");
                           if (!maybe_y.first) {
                             return false;
                           }
                           rectangle_y = maybe_y.second;
                           auto maybe_width = ParseUint32("width");
                           if (!maybe_width.first) {
                             return false;
                           }
                           rectangle_width = maybe_width.second;
                           auto maybe_height = ParseUint32("height");
                           if (!maybe_height.first) {
                             return false;
                           }
                           rectangle_height = maybe_height.second;
                           return true;
                         }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandAssertPixels>(
      std::move(start_token), expected_r, expected_g, expected_b, expected_a,
      renderbuffer_identifier, rectangle_x, rectangle_y, rectangle_width,
      rectangle_height));
  return true;
}

bool Parser::ParseCommandAssertSimilarEmdHistogram() {
  auto start_token = tokenizer_->NextToken();
  std::string buffer_identifier_1;
  std::string buffer_identifier_2;
  float tolerance;
  if (!ParseParameters(
          {{Token::Type::kKeywordBuffer1,
            [this, &buffer_identifier_1]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected identifier for first buffer to be compared");
              }
              buffer_identifier_1 = token->GetText();
              return true;
            }},
           {Token::Type::kKeywordBuffer2,
            [this, &buffer_identifier_2]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected identifier for second buffer to be compared");
              }
              buffer_identifier_2 = token->GetText();
              return true;
            }},
           {Token::Type::kKeywordTolerance, [this, &tolerance]() -> bool {
              auto maybe_tolerance = ParseFloat("tolerance");
              if (!maybe_tolerance.first) {
                return false;
              }
              tolerance = maybe_tolerance.second;
              return true;
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandAssertSimilarEmdHistogram>(
      std::move(start_token), buffer_identifier_1, buffer_identifier_2,
      tolerance));
  return true;
}

bool Parser::ParseCommandBindSampler() {
  auto start_token = tokenizer_->NextToken();
  std::string sampler_identifier;
  size_t texture_unit;
  if (!ParseParameters(
          {{Token::Type::kKeywordSampler,
            [this, &sampler_identifier]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected identifier for the sampler being bound, got '" +
                        token->GetText() + "'");
                return false;
              }
              sampler_identifier = token->GetText();
              return true;
            }},
           {Token::Type::kKeywordTextureUnit, [this, &texture_unit]() -> bool {
              auto maybe_texture_unit = ParseUint32("texture unit");
              if (!maybe_texture_unit.first) {
                return false;
              }
              texture_unit = maybe_texture_unit.second;
              return true;
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandBindSampler>(
      std::move(start_token), sampler_identifier, texture_unit));
  return true;
}

bool Parser::ParseCommandBindStorageBuffer() {
  auto start_token = tokenizer_->NextToken();
  std::string buffer_identifier;
  size_t binding;
  if (!ParseParameters(
          {{Token::Type::kKeywordBuffer,
            [this, &buffer_identifier]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected identifier for storage buffer, got '" +
                        token->GetText() + "'");
                return false;
              }
              buffer_identifier = token->GetText();
              return true;
            }},
           {Token::Type::kKeywordBinding, [this, &binding]() -> bool {
              auto maybe_binding = ParseUint32("binding");
              if (!maybe_binding.first) {
                return false;
              }
              binding = maybe_binding.second;
              return true;
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandBindStorageBuffer>(
      std::move(start_token), buffer_identifier, binding));
  return true;
}

bool Parser::ParseCommandBindTexture() {
  auto start_token = tokenizer_->NextToken();
  std::string texture_identifier;
  size_t texture_unit;
  if (!ParseParameters(
          {{Token::Type::kKeywordTexture,
            [this, &texture_identifier]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected identifier for the sampler being bound, got '" +
                        token->GetText() + "'");
                return false;
              }
              texture_identifier = token->GetText();
              return true;
            }},
           {Token::Type::kKeywordTextureUnit, [this, &texture_unit]() -> bool {
              auto maybe_texture_unit = ParseUint32("texture unit");
              if (!maybe_texture_unit.first) {
                return false;
              }
              texture_unit = maybe_texture_unit.second;
              return true;
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandBindTexture>(
      std::move(start_token), texture_identifier, texture_unit));
  return true;
}

bool Parser::ParseCommandBindUniformBuffer() {
  auto start_token = tokenizer_->NextToken();
  std::string buffer_identifier;
  size_t binding;
  if (!ParseParameters(
          {{Token::Type::kKeywordBuffer,
            [this, &buffer_identifier]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected identifier for uniform buffer, got '" +
                        token->GetText() + "'");
                return false;
              }
              buffer_identifier = token->GetText();
              return true;
            }},
           {Token::Type::kKeywordBinding, [this, &binding]() -> bool {
              auto maybe_binding = ParseUint32("binding");
              if (!maybe_binding.first) {
                return false;
              }
              binding = maybe_binding.second;
              return true;
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandBindUniformBuffer>(
      std::move(start_token), buffer_identifier, binding));
  return true;
}

bool Parser::ParseCommandCompileShader() {
  auto start_token = tokenizer_->NextToken();
  std::unique_ptr<Token> result_identifier = tokenizer_->NextToken();
  if (!result_identifier->IsIdentifier()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, result_identifier.get(),
        "Expected an identifier for the shader being compiled, got '" +
            result_identifier->GetText() + "'");
    return false;
  }
  auto shader_token = tokenizer_->NextToken();
  if (shader_token->GetType() != Token::Type::kKeywordShader) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, shader_token.get(),
        "Expected 'SHADER' keyword, got '" + shader_token->GetText() + "'");
    return false;
  }
  std::unique_ptr<Token> shader_identifier = tokenizer_->NextToken();
  if (!shader_identifier->IsIdentifier()) {
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               shader_identifier.get(),
                               "Expected an identifier for the source of the "
                               "shader being compiled, got '" +
                                   shader_identifier->GetText() + "'");
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandCompileShader>(
      std::move(start_token), std::move(result_identifier),
      std::move(shader_identifier)));
  return true;
}

bool Parser::ParseCommandCreateEmptyTexture2d() {
  auto start_token = tokenizer_->NextToken();
  auto result_identifier = tokenizer_->NextToken();
  if (!result_identifier->IsIdentifier()) {
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               result_identifier.get(),
                               "Expected identifier for texture, got '" +
                                   result_identifier->GetText() + "'");
    return false;
  }
  size_t width;
  size_t height;
  if (!ParseParameters(
          {{Token::Type::kKeywordWidth,
            [this, &width]() -> bool {
              auto maybe_width = ParseUint32("height");
              if (!maybe_width.first) {
                return false;
              }
              width = maybe_width.second;
              return true;
            }},
           {Token::Type::kKeywordHeight, [this, &height]() -> bool {
              auto maybe_height = ParseUint32("height");
              if (!maybe_height.first) {
                return false;
              }
              height = maybe_height.second;
              return true;
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandCreateEmptyTexture2D>(
      std::move(start_token), std::move(result_identifier), width, height));
  return true;
}

bool Parser::ParseCommandCreateBuffer() {
  auto start_token = tokenizer_->NextToken();
  auto result_identifier = tokenizer_->NextToken();
  if (!result_identifier->IsIdentifier()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, result_identifier.get(),
        "Expected an identifier for the buffer being created, got '" +
            result_identifier->GetText() + "'");
    return false;
  }
  size_t size_bytes;
  std::vector<std::unique_ptr<Token>> values;
  CommandCreateBuffer::InitialDataType type;
  if (!ParseParameters(
          {{Token::Type::kKeywordSizeBytes,
            [this, &size_bytes]() -> bool {
              auto maybe_size = ParseUint32("size");
              if (!maybe_size.first) {
                return false;
              }
              size_bytes = maybe_size.second;
              return true;
            }},
           {Token::Type::kKeywordInitType,
            [this, &type]() -> bool {
              auto token = tokenizer_->NextToken();
              if (token->GetText() == "byte") {
                type = CommandCreateBuffer::InitialDataType::kByte;
              } else if (token->GetText() == "float") {
                type = CommandCreateBuffer::InitialDataType::kFloat;
              } else if (token->GetText() == "int") {
                type = CommandCreateBuffer::InitialDataType::kInt;
              } else if (token->GetText() == "uint") {
                type = CommandCreateBuffer::InitialDataType::kUint;
              } else {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "The type for buffer initialization must be one of "
                    "'float', 'int' or 'uint', got '" +
                        token->GetText() + "'");
                return false;
              }
              return true;
            }},
           {Token::Type::kKeywordInitValues, [this, &values]() -> bool {
              while (tokenizer_->PeekNextToken()->IsIntLiteral() ||
                     tokenizer_->PeekNextToken()->IsFloatLiteral()) {
                values.push_back(tokenizer_->NextToken());
              }
              return true;
            }}})) {
    return false;
  }
  size_t element_size =
      type == CommandCreateBuffer::InitialDataType::kByte ? 1U : 4U;
  if (size_bytes != element_size * values.size()) {
    std::stringstream stringstream;
    stringstream << "Size mismatch: buffer '" << result_identifier->GetText()
                 << "' declared with size " << size_bytes
                 << " bytes, but initialized with " << values.size() << " "
                 << element_size << "-byte elements";
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               start_token.get(), stringstream.str());
    return false;
  }
  switch (type) {
    case CommandCreateBuffer::InitialDataType::kByte: {
      std::vector<uint8_t> byte_data;
      for (const auto& value : values) {
        if (!value->IsIntLiteral()) {
          message_consumer_->Message(
              MessageConsumer::Severity::kError, value.get(),
              "Byte literal expected, got '" + value->GetText() + "'");
          return false;
        }
        int32_t parsed_value = std::stoi(value->GetText());
        if (parsed_value < 0 || parsed_value >= UINT8_MAX) {
          message_consumer_->Message(
              MessageConsumer::Severity::kError, value.get(),
              "Byte literal in range [0, 255] expected, got '" +
                  value->GetText() + "'");
          return false;
        }
        byte_data.emplace_back(static_cast<uint8_t>(parsed_value));
      }
      parsed_commands_.push_back(MakeUnique<CommandCreateBuffer>(
          std::move(start_token), std::move(result_identifier), size_bytes,
          byte_data));
      break;
    }
    case CommandCreateBuffer::InitialDataType::kFloat: {
      std::vector<float> float_data;
      for (const auto& value : values) {
        if (!value->IsFloatLiteral()) {
          message_consumer_->Message(
              MessageConsumer::Severity::kError, value.get(),
              "Expected float literal, got '" + value->GetText() + "'");
          return false;
        }
        float_data.emplace_back(std::stof(value->GetText()));
      }
      parsed_commands_.push_back(MakeUnique<CommandCreateBuffer>(
          std::move(start_token), std::move(result_identifier), size_bytes,
          float_data));
      break;
    }
    case CommandCreateBuffer::InitialDataType::kInt: {
      std::vector<int32_t> int_data;
      for (const auto& value : values) {
        if (!value->IsIntLiteral()) {
          message_consumer_->Message(
              MessageConsumer::Severity::kError, value.get(),
              "Expected int literal, got '" + value->GetText() + "'");
          return false;
        }
        int_data.emplace_back(std::stoi(value->GetText()));
      }
      parsed_commands_.push_back(MakeUnique<CommandCreateBuffer>(
          std::move(start_token), std::move(result_identifier), size_bytes,
          int_data));
      break;
    }
    default: {
      assert(type == CommandCreateBuffer::InitialDataType::kUint &&
             "Unexpected type.");
      std::vector<uint32_t> uint_data;
      for (const auto& value : values) {
        if (!value->IsIntLiteral()) {
          message_consumer_->Message(
              MessageConsumer::Severity::kError, value.get(),
              "Expected uint literal, got '" + value->GetText() + "'");
          return false;
        }
        int64_t uint_value = std::stoi(value->GetText());
        if (uint_value < 0) {
          message_consumer_->Message(
              MessageConsumer::Severity::kError, value.get(),
              "uint literal value cannot be negative, got '" +
                  value->GetText() + "'");
          return false;
        }
        if (uint_value > UINT32_MAX) {
          message_consumer_->Message(
              MessageConsumer::Severity::kError, value.get(),
              "uint literal out of range, got '" + value->GetText() + "'");
          return false;
        }
        uint_data.emplace_back(static_cast<uint32_t>(uint_value));
      }
      parsed_commands_.push_back(MakeUnique<CommandCreateBuffer>(
          std::move(start_token), std::move(result_identifier), size_bytes,
          uint_data));
      break;
    }
  }
  return true;
}

bool Parser::ParseCommandCreateProgram() {
  auto start_token = tokenizer_->NextToken();
  auto result_identifier = tokenizer_->NextToken();
  if (!result_identifier->IsIdentifier()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, result_identifier.get(),
        "Expected an identifier for the program being created, got '" +
            result_identifier->GetText() + "'");
    return false;
  }
  auto shaders_token = tokenizer_->NextToken();
  if (shaders_token->GetType() != Token::Type::kKeywordShaders) {
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               shaders_token.get(),
                               "Expected keyword 'SHADERS' before the series "
                               "of compiled shaders for the program, got '" +
                                   shaders_token->GetText() + "'");
    return false;
  }
  auto should_be_first_shader = tokenizer_->PeekNextToken();
  if (!should_be_first_shader->IsIdentifier()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, should_be_first_shader.get(),
        "Expected the identifier of at least one compiled shader, got '" +
            should_be_first_shader->GetText() + "'");
    return false;
  }
  std::vector<std::unique_ptr<Token>> compiled_shader_identifiers;
  while (tokenizer_->PeekNextToken()->IsIdentifier()) {
    compiled_shader_identifiers.push_back(tokenizer_->NextToken());
  }
  parsed_commands_.push_back(MakeUnique<CommandCreateProgram>(
      std::move(start_token), std::move(result_identifier),
      std::move(compiled_shader_identifiers)));
  return true;
}

bool Parser::ParseCommandCreateRenderbuffer() {
  auto start_token = tokenizer_->NextToken();
  auto result_identifier = tokenizer_->NextToken();
  if (!result_identifier->IsIdentifier()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, result_identifier.get(),
        "Expected an identifier for the program being created, got '" +
            result_identifier->GetText() + "'");
    return false;
  }
  size_t width;
  size_t height;
  if (!ParseParameters(
          {{Token::Type::kKeywordWidth,
            [this, &width]() -> bool {
              auto maybe_width = ParseUint32("width");
              if (!maybe_width.first) {
                return false;
              }
              width = maybe_width.second;
              return true;
            }},
           {Token::Type::kKeywordHeight, [this, &height]() -> bool {
              auto maybe_height = ParseUint32("width");
              if (!maybe_height.first) {
                return false;
              }
              height = maybe_height.second;
              return true;
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandCreateRenderbuffer>(
      std::move(start_token), std::move(result_identifier), width, height));
  return true;
}

bool Parser::ParseCommandCreateSampler() {
  auto start_token = tokenizer_->NextToken();
  auto token = tokenizer_->NextToken();
  if (!token->IsIdentifier()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, token.get(),
        "Expected identifier for the sampler being created, got '" +
            token->GetText() + "'");
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandCreateSampler>(
      std::move(start_token), std::move(token)));
  return true;
}

bool Parser::ParseCommandRunCompute() {
  auto start_token = tokenizer_->NextToken();

  std::string program_identifier;
  size_t num_groups_x;
  size_t num_groups_y;
  size_t num_groups_z;

  if (!ParseParameters(
          {{Token::Type::kKeywordProgram,
            [this, &program_identifier]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(MessageConsumer::Severity::kError,
                                           token.get(),
                                           "Expected an identifier for the "
                                           "compute program to be run, got '" +
                                               token->GetText() + "'");
                return false;
              }
              program_identifier = token->GetText();
              return true;
            }},
           {Token::Type::kKeywordNumGroupsX,
            [this, &num_groups_x]() -> bool {
              auto maybe_num_groups_x = ParseUint32("number of groups");
              if (!maybe_num_groups_x.first) {
                return false;
              }
              num_groups_x = maybe_num_groups_x.second;
              return true;
            }},
           {Token::Type::kKeywordNumGroupsY,
            [this, &num_groups_y]() -> bool {
              auto maybe_num_groups_y = ParseUint32("number of groups");
              if (!maybe_num_groups_y.first) {
                return false;
              }
              num_groups_y = maybe_num_groups_y.second;
              return true;
            }},
           {Token::Type::kKeywordNumGroupsZ, [this, &num_groups_z]() -> bool {
              auto maybe_num_groups_z = ParseUint32("number of groups");
              if (!maybe_num_groups_z.first) {
                return false;
              }
              num_groups_z = maybe_num_groups_z.second;
              return true;
            }}})) {
    return false;
  }
  parsed_commands_.push_back(
      MakeUnique<CommandRunCompute>(std::move(start_token), program_identifier,
                                    num_groups_x, num_groups_y, num_groups_z));
  return true;
}

bool Parser::ParseCommandRunGraphics() {
  auto start_token = tokenizer_->NextToken();

  std::string program_identifier;
  std::unordered_map<size_t, VertexAttributeInfo> vertex_data;
  std::string index_data_buffer_identifier;
  size_t vertex_count;
  CommandRunGraphics::Topology topology;
  std::unordered_map<size_t, std::string> framebuffer_attachments;

  if (!ParseParameters(
          {{Token::Type::kKeywordProgram,
            [this, &program_identifier]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(MessageConsumer::Severity::kError,
                                           token.get(),
                                           "Expected an identifier for the "
                                           "graphics program to be run, got '" +
                                               token->GetText() + "'");
                return false;
              }
              program_identifier = token->GetText();
              return true;
            }},
           {Token::Type::kKeywordVertexData,
            [this, &vertex_data]() -> bool {
              auto token = tokenizer_->NextToken();
              if (token->GetText() != "[") {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected '[' to commence start of vertex data, got '" +
                        token->GetText() + "'");
                return false;
              }
              while (tokenizer_->PeekNextToken()->GetText() != "]") {
                auto maybe_location = ParseUint32("location");
                if (!maybe_location.first) {
                  return false;
                }
                token = tokenizer_->NextToken();
                if (token->GetText() != "->") {
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError, token.get(),
                      "Expected '->', got '" + token->GetText() + "'");
                  return false;
                }
                std::pair<bool, VertexAttributeInfo> maybe_vertex_list =
                    ParseVertexAttributeInfo();
                if (!maybe_vertex_list.first) {
                  return false;
                }
                vertex_data.insert(
                    {maybe_location.second, maybe_vertex_list.second});
                token = tokenizer_->PeekNextToken();
                if (token->GetText() == ",") {
                  tokenizer_->NextToken();
                } else if (token->GetText() != "]") {
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError, token.get(),
                      "Expected ',' or ']', got '" + token->GetText() + "'");
                  return false;
                }
              }
              tokenizer_->NextToken();
              return true;
            }},
           {Token::Type::kKeywordIndexData,
            [this, &index_data_buffer_identifier]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected identifier for index data buffer, got '" +
                        token->GetText() + "'");
                return false;
              }
              index_data_buffer_identifier = token->GetText();
              return true;
            }},
           {Token::Type::kKeywordVertexCount,
            [this, &vertex_count]() -> bool {
              auto maybe_vertex_count = ParseUint32("vertex count");
              if (!maybe_vertex_count.first) {
                return false;
              }
              vertex_count = maybe_vertex_count.second;
              return true;
            }},
           {Token::Type::kKeywordTopology,
            [this, &topology]() -> bool {
              auto token = tokenizer_->NextToken();
              if (token->GetType() == Token::Type::kKeywordTriangles) {
                topology = CommandRunGraphics::Topology::kTriangles;
              } else {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Unknown or unsupported topology: '" + token->GetText() +
                        "'");
                return false;
              }
              return true;
            }},
           {Token::Type::kKeywordFramebufferAttachments,
            [this, &framebuffer_attachments]() -> bool {
              auto token = tokenizer_->NextToken();
              if (token->GetText() != "[") {
                message_consumer_->Message(MessageConsumer::Severity::kError,
                                           token.get(),
                                           "Expected '[' to commence start of "
                                           "framebuffer attachments, got '" +
                                               token->GetText() + "'");
                return false;
              }
              while (tokenizer_->PeekNextToken()->GetText() != "]") {
                auto maybe_location = ParseUint32("location");
                if (!maybe_location.first) {
                  return false;
                }
                token = tokenizer_->NextToken();
                if (token->GetText() != "->") {
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError, token.get(),
                      "Expected '->', got '" + token->GetText() + "'");
                  return false;
                }
                token = tokenizer_->NextToken();
                if (!token->IsIdentifier()) {
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError, token.get(),
                      "Expected identifier for framebuffer attachment, got '" +
                          token->GetText() + "'");
                  return false;
                }
                framebuffer_attachments.insert(
                    {maybe_location.second, token->GetText()});
                token = tokenizer_->PeekNextToken();
                if (token->GetText() == ",") {
                  tokenizer_->NextToken();
                } else if (token->GetText() != "]") {
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError, token.get(),
                      "Expected ',' or ']', got '" + token->GetText() + "'");
                  return false;
                }
              }
              tokenizer_->NextToken();
              return true;
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandRunGraphics>(
      std::move(start_token), program_identifier, vertex_data,
      index_data_buffer_identifier, vertex_count, topology,
      framebuffer_attachments));
  return true;
}

bool Parser::ParseCommandDeclareShader() {
  auto start_token = tokenizer_->NextToken();
  std::unique_ptr<Token> result_identifier = tokenizer_->NextToken();
  if (!result_identifier->IsIdentifier()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, result_identifier.get(),
        "Expected an identifier for the shader being declared, got '" +
            result_identifier->GetText() + "'");
    return false;
  }
  auto shader_kind = tokenizer_->NextToken();
  if (shader_kind->GetType() != Token::Type::kKeywordVertex &&
      shader_kind->GetType() != Token::Type::kKeywordFragment &&
      shader_kind->GetType() != Token::Type::kKeywordCompute) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, shader_kind.get(),
        "Expected 'VERTEX', 'FRAGMENT' or 'COMPUTE' to specify "
        "which kind of shader this is, got '" +
            shader_kind->GetText() + "'");
    return false;
  }
  // The shader text should start on the next line, but there could be
  // whitespace and comments on the rest of this line, so skip over them (but
  // restrict skipping to just this line).
  auto skipped_comment = tokenizer_->SkipSingleLineOfWhitespaceAndComments();
  if (shader_kind->GetLine() == tokenizer_->GetLine()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, shader_kind.get(),
        "Shader text should begin on the line directly following the '" +
            shader_kind->GetText() + "' keyword");
    return false;
  }
  const std::string kVersionString = "#version ";
  if (skipped_comment->GetText().substr(
          0, std::string(kVersionString).length()) == kVersionString) {
    message_consumer_->Message(
        MessageConsumer::Severity::kWarning, skipped_comment.get(),
        "'" + kVersionString +
            "...' will be treated as a comment. If it is supposed to be the "
            "first line of shader code, it should start on the following line");
  }
  const size_t shader_start_line = tokenizer_->GetLine();
  std::stringstream stringstream;
  while (true) {
    auto token = tokenizer_->PeekNextToken(false);
    if (token->IsEOS()) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, token.get(),
          "Unexpected end of script when processing shader text");
      return false;
    }
    if (token->GetText() == "END") {
      break;
    }
    stringstream << tokenizer_->SkipLine();
  }
  tokenizer_->NextToken();
  CommandDeclareShader::Kind declare_shader_kind =
      CommandDeclareShader::Kind::VERTEX;
  switch (shader_kind->GetType()) {
    case Token::Type::kKeywordVertex:
      declare_shader_kind = CommandDeclareShader::Kind::VERTEX;
      break;
    case Token::Type::kKeywordFragment:
      declare_shader_kind = CommandDeclareShader::Kind::FRAGMENT;
      break;
    case Token::Type::kKeywordCompute:
      declare_shader_kind = CommandDeclareShader::Kind::COMPUTE;
      break;
    default:
      assert(false && "Unexpected token type.");
      break;
  }

  parsed_commands_.push_back(MakeUnique<CommandDeclareShader>(
      std::move(start_token), std::move(result_identifier), declare_shader_kind,
      stringstream.str(), shader_start_line));
  return true;
}

bool Parser::ParseCommandDumpRenderbuffer() {
  auto start_token = tokenizer_->NextToken();
  std::string renderbuffer_identifier;
  std::string filename;
  if (!ParseParameters(
          {{Token::Type::kKeywordRenderbuffer,
            [this, &renderbuffer_identifier]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected renderbuffer identifier, got '" +
                        token->GetText() + "'");
                return false;
              }
              renderbuffer_identifier = token->GetText();
              return true;
            }},
           {Token::Type::kKeywordFile, [this, &filename]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsString()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Exected file to which to dump renderbuffer, got '" +
                        token->GetText() + "'");
                return false;
              }
              filename =
                  token->GetText().substr(1, token->GetText().length() - 2);
              return true;
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandDumpRenderbuffer>(
      std::move(start_token), renderbuffer_identifier, filename));
  return true;
}

bool Parser::ParseCommandSetTextureOrSamplerParameter() {
  auto start_token = tokenizer_->NextToken();
  const bool is_set_texture =
      start_token->GetType() == Token::Type::kKeywordSetTextureParameter;
  bool got_target_identifier = false;
  std::string target_identifier;
  bool got_parameter = false;
  CommandSetSamplerOrTextureParameter::TextureParameter parameter =
      CommandSetSamplerOrTextureParameter::TextureParameter::kMagFilter;
  CommandSetSamplerOrTextureParameter::TextureParameterValue parameter_value =
      CommandSetSamplerOrTextureParameter::TextureParameterValue::kNearest;

  while (true) {
    auto token = tokenizer_->PeekNextToken();
    if (token->GetType() == Token::Type::kKeywordSampler ||
        token->GetType() == Token::Type::kKeywordTexture) {
      token = tokenizer_->NextToken();
      Token::Type expected_token = is_set_texture
                                       ? Token::Type::kKeywordTexture
                                       : Token::Type::kKeywordSampler;
      if (token->GetType() != expected_token) {
        message_consumer_->Message(
            MessageConsumer::Severity::kError, token.get(),
            "The 'SET_" + Tokenizer::KeywordToString(expected_token) +
                "_PARAMETER' command takes a '" +
                Tokenizer::KeywordToString(expected_token) +
                "' argument, not a '" + token->GetText() + "' argument");
        return false;
      }
      if (got_target_identifier) {
        message_consumer_->Message(
            MessageConsumer::Severity::kError, token.get(),
            "Multiple '" + Tokenizer::KeywordToString(expected_token) +
                "' parmaters provided, already got '" + target_identifier +
                "'");
        return false;
      }
      token = tokenizer_->NextToken();
      if (!token->IsIdentifier()) {
        std::stringstream stringstream;
        stringstream << "Expected identifier for target "
                     << (expected_token == Token::Type::kKeywordTexture
                             ? "texture"
                             : "sampler")
                     << ", got '" << token->GetText() << "'";
        message_consumer_->Message(MessageConsumer::Severity::kError,
                                   token.get(), stringstream.str());
        return false;
      }
      target_identifier = token->GetText();
      got_target_identifier = true;
    } else if (token->GetType() == Token::Type::kKeywordTextureMagFilter ||
               token->GetType() == Token::Type::kKeywordTextureMinFilter) {
      if (got_parameter) {
        std::stringstream stringstream;
        stringstream << "Multiple parameters specified for "
                     << (is_set_texture ? "texture" : "sampler");
        message_consumer_->Message(MessageConsumer::Severity::kError,
                                   token.get(), stringstream.str());
        return false;
      }
      got_parameter = true;
      auto parameter_name = tokenizer_->NextToken();
      token = tokenizer_->NextToken();
      parameter = parameter_name->GetText() == "TEXTURE_MAG_FILTER"
                      ? CommandSetSamplerOrTextureParameter::TextureParameter::
                            kMagFilter
                      : CommandSetSamplerOrTextureParameter::TextureParameter::
                            kMinFilter;
      if (token->GetText() == "LINEAR") {
        parameter_value =
            CommandSetSamplerOrTextureParameter::TextureParameterValue::kLinear;
      } else if (token->GetText() == "NEAREST") {
        parameter_value = CommandSetSamplerOrTextureParameter::
            TextureParameterValue::kNearest;
      } else {
        message_consumer_->Message(
            MessageConsumer::Severity::kError, token.get(),
            "Expected value for the '" + parameter_name->GetText() +
                "' parameter, got '" + token->GetText());
        return false;
      }
    } else {
      break;
    }
  }
  if (!got_target_identifier) {
    std::stringstream stringstream;
    stringstream << "No target " << (is_set_texture ? "texture" : "sampler")
                 << " was specified";
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               start_token.get(), stringstream.str());
    return false;
  }
  if (!got_parameter) {
    std::stringstream stringstream;
    stringstream << "No " << (is_set_texture ? "texture" : "sampler")
                 << " parameter was specified";
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               start_token.get(), stringstream.str());
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandSetSamplerOrTextureParameter>(
      std::move(start_token), target_identifier, parameter, parameter_value));
  return true;
}

bool Parser::ParseCommandSetUniform() {
  auto start_token = tokenizer_->NextToken();
  std::string program_identifier;
  size_t location;
  UniformValue::ElementType type;
  std::pair<bool, size_t> maybe_array_size;
  std::vector<std::unique_ptr<Token>> values;
  if (!ParseParameters(
          {{Token::Type::kKeywordProgram,
            [this, &program_identifier]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(MessageConsumer::Severity::kError,
                                           token.get(),
                                           "Expected identifier of program for "
                                           "which uniform is to be set, got '" +
                                               token->GetText() + "'");
                return false;
              }
              program_identifier = token->GetText();
              return true;
            }},
           {Token::Type::kKeywordLocation,
            [this, &location]() -> bool {
              auto maybe_location = ParseUint32("location");
              if (!maybe_location.first) {
                return false;
              }
              location = maybe_location.second;
              return true;
            }},
           {Token::Type::kKeywordType,
            [this, &maybe_array_size, &type]() -> bool {
              auto token = tokenizer_->NextToken();
              if (token->GetText() == "float") {
                type = UniformValue::ElementType::kFloat;
              } else if (token->GetText() == "vec2") {
                type = UniformValue::ElementType::kVec2;
              } else if (token->GetText() == "vec3") {
                type = UniformValue::ElementType::kVec3;
              } else if (token->GetText() == "vec4") {
                type = UniformValue::ElementType::kVec4;
              } else if (token->GetText() == "int") {
                type = UniformValue::ElementType::kInt;
              } else if (token->GetText() == "ivec2") {
                type = UniformValue::ElementType::kIvec2;
              } else if (token->GetText() == "ivec3") {
                type = UniformValue::ElementType::kIvec3;
              } else if (token->GetText() == "ivec4") {
                type = UniformValue::ElementType::kIvec4;
              } else if (token->GetText() == "uint") {
                type = UniformValue::ElementType::kUint;
              } else if (token->GetText() == "uvec2") {
                type = UniformValue::ElementType::kUvec2;
              } else if (token->GetText() == "uvec3") {
                type = UniformValue::ElementType::kUvec3;
              } else if (token->GetText() == "uvec4") {
                type = UniformValue::ElementType::kUvec4;
              } else if (token->GetText() == "mat2x2") {
                type = UniformValue::ElementType::kMat2x2;
              } else if (token->GetText() == "mat2x3") {
                type = UniformValue::ElementType::kMat2x3;
              } else if (token->GetText() == "mat2x4") {
                type = UniformValue::ElementType::kMat2x4;
              } else if (token->GetText() == "mat3x2") {
                type = UniformValue::ElementType::kMat3x2;
              } else if (token->GetText() == "mat3x3") {
                type = UniformValue::ElementType::kMat3x3;
              } else if (token->GetText() == "mat3x4") {
                type = UniformValue::ElementType::kMat3x4;
              } else if (token->GetText() == "mat4x2") {
                type = UniformValue::ElementType::kMat4x2;
              } else if (token->GetText() == "mat4x3") {
                type = UniformValue::ElementType::kMat4x3;
              } else if (token->GetText() == "mat4x4") {
                type = UniformValue::ElementType::kMat4x4;
              } else {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Unexpected type '" + token->GetText() + "'");
                return false;
              }
              if (tokenizer_->PeekNextToken()->GetText() == "[") {
                tokenizer_->NextToken();
                auto maybe_size = ParseUint32("array size");
                if (!maybe_size.first) {
                  return false;
                }
                maybe_array_size = {true, maybe_array_size.second};
                token = tokenizer_->NextToken();
                if (token->GetText() != "]") {
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError, token.get(),
                      "Expected ']', got '" + token->GetText() + "'");
                }
              } else {
                maybe_array_size = {false, 0U};
              }
              return true;
            }},
           {Token::Type::kKeywordValues, [this, &values]() -> bool {
              while (tokenizer_->PeekNextToken()->IsIntLiteral() ||
                     tokenizer_->PeekNextToken()->IsFloatLiteral()) {
                values.push_back(tokenizer_->NextToken());
              }
              return true;
            }}})) {
    return false;
  }
  auto maybe_uniform_value =
      ProcessUniformValue(type, maybe_array_size, values);
  if (!maybe_uniform_value.first) {
    return false;
  }
  parsed_commands_.push_back(
      MakeUnique<CommandSetUniform>(std::move(start_token), program_identifier,
                                    location, maybe_uniform_value.second));
  return true;
}

std::pair<bool, UniformValue> Parser::ProcessUniformValue(
    UniformValue::ElementType type,
    const std::pair<bool, size_t>& maybe_array_size,
    const std::vector<std::unique_ptr<Token>>& values) {
  std::pair<bool, UniformValue> failure_result{
      false,
      UniformValue(UniformValue::ElementType::kFloat, std::vector<float>())};
  if (!ParseParameters({})) {
    return failure_result;
  }
  switch (type) {
    case UniformValue::ElementType::kInt:
    case UniformValue::ElementType::kIvec2:
    case UniformValue::ElementType::kIvec3:
    case UniformValue::ElementType::kIvec4: {
      std::vector<int32_t> int_values;
      for (const auto& value : values) {
        if (!value->IsIntLiteral()) {
          message_consumer_->Message(
              MessageConsumer::Severity::kError, value.get(),
              "Found non-integer value '" + value->GetText() +
                  "' for integer uniform");
          return failure_result;
        }
        int_values.push_back(std::stoi(value->GetText()));
      }
      if (maybe_array_size.first) {
        return {true, UniformValue(type, int_values, maybe_array_size.second)};
      }
      return {true, UniformValue(type, int_values)};
    }
    case UniformValue::ElementType::kUint:
    case UniformValue::ElementType::kUvec2:
    case UniformValue::ElementType::kUvec3:
    case UniformValue::ElementType::kUvec4: {
      std::vector<uint32_t> uint_values;
      for (const auto& value : values) {
        if (!value->IsIntLiteral() || std::stoi(value->GetText()) < 0) {
          message_consumer_->Message(MessageConsumer::Severity::kError,
                                     value.get(),
                                     "An unsigned uniform requires a "
                                     "non-negative integer value, got '" +
                                         value->GetText() + "'");
          return failure_result;
        }
        uint_values.push_back(
            static_cast<uint32_t>(std::stoi(value->GetText())));
      }
      if (maybe_array_size.first) {
        return {true, UniformValue(type, uint_values, maybe_array_size.second)};
      }
      return {true, UniformValue(type, uint_values)};
    }
    default: {
      std::vector<float> float_values;
      for (const auto& value : values) {
        if (!value->IsFloatLiteral()) {
          message_consumer_->Message(
              MessageConsumer::Severity::kError, value.get(),
              "Found non-float value '" + value->GetText() +
                  "' for float uniform");
          return failure_result;
        }
        float_values.push_back(std::stof(value->GetText()));
      }
      if (maybe_array_size.first) {
        return {true,
                UniformValue(type, float_values, maybe_array_size.second)};
      }
      return {true, UniformValue(type, float_values)};
    }
  }
}

bool Parser::ParseParameters(
    const std::map<Token::Type, std::function<bool()>>& parameter_parsers) {
  std::set<Token::Type> observed;
  while (true) {
    auto token = tokenizer_->PeekNextToken();
    if (parameter_parsers.count(token->GetType()) == 0) {
      break;
    }
    if (observed.count(token->GetType()) > 0) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, token.get(),
          "Duplicate parameter '" + token->GetText() + "'");
      return false;
    }
    observed.insert(token->GetType());
    tokenizer_->NextToken();
    if (!parameter_parsers.at(token->GetType())()) {
      return false;
    }
  }
  bool all_parameters_present = true;
  for (const auto& entry : parameter_parsers) {
    if (observed.count(entry.first) == 0) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, tokenizer_->PeekNextToken().get(),
          "Missing parameter '" + Tokenizer::KeywordToString(entry.first) +
              "'");
      all_parameters_present = false;
    }
  }
  return all_parameters_present;
}

std::pair<bool, VertexAttributeInfo> Parser::ParseVertexAttributeInfo() {
  std::string buffer_identifier;
  size_t offset_bytes;
  size_t stride_bytes;
  size_t dimension;
  if (!ParseParameters(
          {{Token::Type::kKeywordBuffer,
            [this, &buffer_identifier]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected identifier for vertex buffer, got '" +
                        token->GetText() + "'");
                return false;
              }
              buffer_identifier = token->GetText();
              return true;
            }},
           {Token::Type::kKeywordOffsetBytes,
            [this, &offset_bytes]() -> bool {
              auto maybe_offset = ParseUint32("offset");
              if (!maybe_offset.first) {
                return false;
              }
              offset_bytes = maybe_offset.second;
              return true;
            }},
           {Token::Type::kKeywordStrideBytes,
            [this, &stride_bytes]() -> bool {
              auto maybe_stride = ParseUint32("stride");
              if (!maybe_stride.first) {
                return false;
              }
              stride_bytes = maybe_stride.second;
              return true;
            }},
           {Token::Type::kKeywordDimension, [this, &dimension]() -> bool {
              auto maybe_dimension = ParseUint32("dimension");
              if (!maybe_dimension.first) {
                return false;
              }
              dimension = maybe_dimension.second;
              return true;
            }}})) {
    return {false, VertexAttributeInfo("", 0, 0, 0)};
  }
  return {true, VertexAttributeInfo(buffer_identifier, offset_bytes,
                                    stride_bytes, dimension)};
}

std::pair<bool, uint8_t> Parser::ParseUint8(const std::string& result_name) {
  auto token = tokenizer_->NextToken();
  if (!token->IsIntLiteral()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, token.get(),
        "Expected integer " + result_name + ", got '" + token->GetText() + "'");
    return {false, static_cast<uint8_t>(0U)};
  }
  int64_t result = std::stol(token->GetText());
  if (result < 0 || result > UINT8_MAX) {
    message_consumer_->Message(MessageConsumer::Severity::kError, token.get(),
                               "Expected integer " + result_name +
                                   " in the range [0, 255], got '" +
                                   token->GetText() + "'");
    return {false, static_cast<uint8_t>(0U)};
  }
  return {true, static_cast<uint8_t>(result)};
}

std::pair<bool, uint32_t> Parser::ParseUint32(const std::string& result_name) {
  auto token = tokenizer_->NextToken();
  if (!token->IsIntLiteral()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, token.get(),
        "Expected integer " + result_name + ", got '" + token->GetText() + "'");
    return {false, 0U};
  }
  int64_t result = std::stol(token->GetText());
  if (result < 0) {
    message_consumer_->Message(MessageConsumer::Severity::kError, token.get(),
                               "Expected non-negative integer " + result_name +
                                   ", got '" + token->GetText() + "'");
    return {false, 0U};
  }
  if (result > UINT32_MAX) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, token.get(),
        "Value '" + token->GetText() + "' is out of range");
    return {false, 0U};
  }
  return {true, static_cast<uint32_t>(result)};
}

std::pair<bool, float> Parser::ParseFloat(const std::string& result_name) {
  auto token = tokenizer_->NextToken();
  if (!token->IsFloatLiteral()) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, token.get(),
        "Expected float " + result_name + ", got '" + token->GetText() + "'");
    return {false, 0.0F};
  }
  return {true, std::stof(token->GetText())};
}

std::unique_ptr<ShaderTrapProgram> Parser::GetParsedProgram() {
  return MakeUnique<ShaderTrapProgram>(std::move(parsed_commands_));
}

}  // namespace shadertrap
