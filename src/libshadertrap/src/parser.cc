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
#include <initializer_list>
#include <numeric>
#include <set>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <utility>

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
#include "libshadertrap/make_unique.h"
#include "libshadertrap/texture_parameter.h"
#include "libshadertrap/token.h"
#include "libshadertrap/tokenizer.h"

namespace shadertrap {

Parser::Parser(const std::string& input, MessageConsumer* message_consumer)
    : tokenizer_(MakeUnique<Tokenizer>(input)),
      message_consumer_(message_consumer) {}

Parser::~Parser() = default;

bool Parser::Parse() {
  if (!ParseApiVersion()) {
    return false;
  }
  while (!tokenizer_->PeekNextToken()->IsEOS()) {
    if (!ParseCommand()) {
      return false;
    }
  }
  return true;
}

bool Parser::ParseApiVersion() {
  assert(api_version_ == nullptr && "API version should not yet be set");
  auto api_token = tokenizer_->NextToken();
  ApiVersion::Api api;
  switch (api_token->GetType()) {
    case Token::Type::kKeywordGl:
      api = ApiVersion::Api::GL;
      break;
    case Token::Type::kKeywordGles:
      api = ApiVersion::Api::GLES;
      break;
    default:
      message_consumer_->Message(MessageConsumer::Severity::kError,
                                 api_token.get(),
                                 "Expected API version to begin with 'GL' for "
                                 "OpenGL or 'GLES' for OpenGL ES; found '" +
                                     api_token->GetText() + "'");
      return false;
  }
  auto major_minor = tokenizer_->NextToken();
  if (major_minor->GetType() != Token::Type::kFloatLiteral) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, api_token.get(),
        "Expected major and minor versions in the form 'MAJOR.MINOR'; found '" +
            major_minor->GetText() + "'");
    return false;
  }
  uint32_t major;
  uint32_t minor;
  if (api == ApiVersion::Api::GL) {
    if (major_minor->GetText() == "4.0") {
      major = 4;
      minor = 0;
    } else if (major_minor->GetText() == "4.1") {
      major = 4;
      minor = 1;
    } else if (major_minor->GetText() == "4.2") {
      major = 4;
      minor = 2;
    } else if (major_minor->GetText() == "4.3") {
      major = 4;
      minor = 3;
    } else if (major_minor->GetText() == "4.4") {
      major = 4;
      minor = 4;
    } else if (major_minor->GetText() == "4.5") {
      major = 4;
      minor = 5;  // NOLINT
    } else if (major_minor->GetText() == "4.6") {
      major = 4;
      minor = 6;  // NOLINT
    } else {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, api_token.get(),
          "Unsupported OpenGL version: " + major_minor->GetText());
      return false;
    }
  } else {
    if (major_minor->GetText() == "2.0") {
      major = 2;
      minor = 0;
    } else if (major_minor->GetText() == "3.0") {
      major = 3;
      minor = 0;
    } else if (major_minor->GetText() == "3.1") {
      major = 3;
      minor = 1;
    } else if (major_minor->GetText() == "3.2") {
      major = 3;
      minor = 2;
    } else {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, api_token.get(),
          "Unsupported OpenGL ES version: " + major_minor->GetText());
      return false;
    }
  }
  api_version_ = MakeUnique<ApiVersion>(api, major, minor);
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
    case Token::Type::kKeywordBindShaderStorageBuffer:
      return ParseCommandBindShaderStorageBuffer();
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
    case Token::Type::kKeywordDumpBufferBinary:
      return ParseCommandDumpBufferBinary();
    case Token::Type::kKeywordDumpBufferText:
      return ParseCommandDumpBufferText();
    case Token::Type::kKeywordDumpRenderbuffer:
      return ParseCommandDumpRenderbuffer();
    case Token::Type::kKeywordRunCompute:
      return ParseCommandRunCompute();
    case Token::Type::kKeywordRunGraphics:
      return ParseCommandRunGraphics();
    case Token::Type::kKeywordSetSamplerParameter:
      return ParseCommandSetSamplerParameter();
    case Token::Type::kKeywordSetTextureParameter:
      return ParseCommandSetTextureParameter();
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
  std::unique_ptr<Token> argument_identifier_1;
  std::unique_ptr<Token> argument_identifier_2;
  bool arguments_are_renderbuffers = false;
  std::vector<CommandAssertEqual::FormatEntry> format_entries;
  if (!ParseParameters(
          {{Token::Type::kKeywordBuffers,
            [this, &arguments_are_renderbuffers, &argument_identifier_1,
             &argument_identifier_2]() -> bool {
              arguments_are_renderbuffers = false;
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected identifier for first buffer "
                    "to be compared");
              }
              argument_identifier_1 = std::move(token);
              token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected identifier for second buffer "
                    "to be compared");
              }
              argument_identifier_2 = std::move(token);
              return true;
            }},
           {Token::Type::kKeywordRenderbuffers,
            [this, &arguments_are_renderbuffers, &argument_identifier_1,
             &argument_identifier_2]() -> bool {
              arguments_are_renderbuffers = true;
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected identifier for first renderbuffer "
                    "to be compared");
              }
              argument_identifier_1 = std::move(token);
              token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected identifier for second renderbuffer "
                    "to be compared");
              }
              argument_identifier_2 = std::move(token);
              return true;
            }},

           {Token::Type::kKeywordFormat,
            [this, &format_entries, &start_token]() -> bool {
              bool seen_at_least_one_format_entry = false;
              while (true) {
                CommandAssertEqual::FormatEntry::Kind kind;
                switch (tokenizer_->PeekNextToken()->GetType()) {
                  case Token::Type::kKeywordSkipBytes:
                    seen_at_least_one_format_entry = true;
                    kind = CommandAssertEqual::FormatEntry::Kind::kSkip;
                    break;
                  case Token::Type::kKeywordTypeByte:
                    seen_at_least_one_format_entry = true;
                    kind = CommandAssertEqual::FormatEntry::Kind::kByte;
                    break;
                  case Token::Type::kKeywordTypeFloat:
                    seen_at_least_one_format_entry = true;
                    kind = CommandAssertEqual::FormatEntry::Kind::kFloat;
                    break;
                  case Token::Type::kKeywordTypeInt:
                    seen_at_least_one_format_entry = true;
                    kind = CommandAssertEqual::FormatEntry::Kind::kInt;
                    break;
                  case Token::Type::kKeywordTypeUint:
                    seen_at_least_one_format_entry = true;
                    kind = CommandAssertEqual::FormatEntry::Kind::kUint;
                    break;
                  default:
                    // Handles the case when no identifier is specified after
                    // FORMAT.
                    if (!seen_at_least_one_format_entry) {
                      message_consumer_->Message(
                          MessageConsumer::Severity::kError, start_token.get(),
                          "Missing identifier after FORMAT");
                    }
                    return seen_at_least_one_format_entry;
                }
                auto format_start_token = tokenizer_->NextToken();
                size_t count;
                auto maybe_count = ParseUint32("count");

                // Handles the case when the count after the FORMAT option is 0
                // or is not specified
                if (!maybe_count.first) {
                  return false;
                }
                count = maybe_count.second;

                format_entries.push_back(
                    {std::move(format_start_token), kind, count});
              }
            }}},
          // BUFFERS and RENDERBUFFERS are mutually exclusive parameters
          {{Token::Type::kKeywordBuffers, Token::Type::kKeywordRenderbuffers}},

          {Token::Type::kKeywordFormat})) {
    return false;
  }
  if (arguments_are_renderbuffers) {
    if (!format_entries.empty()) {
      message_consumer_->Message(MessageConsumer::Severity::kError,
                                 start_token.get(),
                                 "FORMAT specifier cannot be set"
                                 "for renderbuffers arguments");
      return false;
    }
    parsed_commands_.push_back(MakeUnique<CommandAssertEqual>(
        std::move(start_token), std::move(argument_identifier_1),
        std::move(argument_identifier_2)));
    return true;
  }

  parsed_commands_.push_back(MakeUnique<CommandAssertEqual>(
      std::move(start_token), std::move(argument_identifier_1),
      std::move(argument_identifier_2), std::move(format_entries)));

  return true;
}

bool Parser::ParseCommandAssertPixels() {
  auto start_token = tokenizer_->NextToken();
  uint8_t expected_r;
  uint8_t expected_g;
  uint8_t expected_b;
  uint8_t expected_a;
  std::unique_ptr<Token> renderbuffer_identifier;
  size_t rectangle_x;
  size_t rectangle_y;
  size_t rectangle_width;
  size_t rectangle_height;
  std::unique_ptr<Token> rectangle_width_token;
  std::unique_ptr<Token> rectangle_height_token;
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
                           renderbuffer_identifier = std::move(token);
                           return true;
                         }},
                        {Token::Type::kKeywordRectangle,
                         [this, &rectangle_x, &rectangle_y, &rectangle_width,
                          &rectangle_height, &rectangle_width_token,
                          &rectangle_height_token]() -> bool {
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
                           rectangle_width_token = tokenizer_->PeekNextToken();
                           auto maybe_width = ParseUint32("width");
                           if (!maybe_width.first) {
                             return false;
                           }
                           rectangle_width = maybe_width.second;
                           rectangle_height_token = tokenizer_->PeekNextToken();
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
      std::move(renderbuffer_identifier), rectangle_x, rectangle_y,
      rectangle_width, rectangle_height, std::move(rectangle_width_token),
      std::move(rectangle_height_token)));
  return true;
}

bool Parser::ParseCommandAssertSimilarEmdHistogram() {
  auto start_token = tokenizer_->NextToken();
  std::unique_ptr<Token> renderbuffer_identifier_1;
  std::unique_ptr<Token> renderbuffer_identifier_2;
  float tolerance;
  if (!ParseParameters(
          {{Token::Type::kKeywordRenderbuffers,
            [this, &renderbuffer_identifier_1,
             &renderbuffer_identifier_2]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(MessageConsumer::Severity::kError,
                                           token.get(),
                                           "Expected identifier for first "
                                           "renderbuffer to be compared");
              }
              renderbuffer_identifier_1 = std::move(token);
              token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(MessageConsumer::Severity::kError,
                                           token.get(),
                                           "Expected identifier for second "
                                           "renderbuffer to be compared");
              }
              renderbuffer_identifier_2 = std::move(token);
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
      std::move(start_token), std::move(renderbuffer_identifier_1),
      std::move(renderbuffer_identifier_2), tolerance));
  return true;
}

bool Parser::ParseCommandBindSampler() {
  auto start_token = tokenizer_->NextToken();
  std::unique_ptr<Token> sampler_identifier;
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
              sampler_identifier = std::move(token);
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
      std::move(start_token), std::move(sampler_identifier), texture_unit));
  return true;
}

bool Parser::ParseCommandBindShaderStorageBuffer() {
  auto start_token = tokenizer_->NextToken();
  std::unique_ptr<Token> buffer_identifier;
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
              buffer_identifier = std::move(token);
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
  parsed_commands_.push_back(MakeUnique<CommandBindShaderStorageBuffer>(
      std::move(start_token), std::move(buffer_identifier), binding));
  return true;
}

bool Parser::ParseCommandBindTexture() {
  auto start_token = tokenizer_->NextToken();
  std::unique_ptr<Token> texture_identifier;
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
              texture_identifier = std::move(token);
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
      std::move(start_token), std::move(texture_identifier), texture_unit));
  return true;
}

bool Parser::ParseCommandBindUniformBuffer() {
  auto start_token = tokenizer_->NextToken();
  std::unique_ptr<Token> buffer_identifier;
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
              buffer_identifier = std::move(token);
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
      std::move(start_token), std::move(buffer_identifier), binding));
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
  std::vector<ValuesSegment> values;
  std::unique_ptr<Token> size_in_bytes_token = nullptr;
  if (!ParseParameters(
          {{Token::Type::kKeywordSizeBytes,
            [this, &size_bytes, &size_in_bytes_token]() -> bool {
              size_in_bytes_token = tokenizer_->PeekNextToken();
              auto maybe_size = ParseUint32("size");
              if (!maybe_size.first) {
                return false;
              }
              size_bytes = maybe_size.second;
              return true;
            }},
           {Token::Type::kKeywordInitValues, [this, &values]() -> bool {
              while (true) {
                switch (tokenizer_->PeekNextToken()->GetType()) {
                  case Token::Type::kKeywordTypeByte:
                  case Token::Type::kKeywordTypeFloat:
                  case Token::Type::kKeywordTypeInt:
                  case Token::Type::kKeywordTypeUint: {
                    std::pair<bool, ValuesSegment> maybe_values_segment =
                        ParseValuesSegment();
                    if (!maybe_values_segment.first) {
                      return false;
                    }
                    values.push_back(maybe_values_segment.second);
                    break;
                  }
                  default:
                    return true;
                }
              }
            }}})) {
    return false;
  }
  size_t actual_size =
      std::accumulate(values.begin(), values.end(), size_t{0U},
                      [](size_t a, const ValuesSegment& segment) {
                        return a + segment.GetSizeBytes();
                      });
  if (size_bytes != actual_size) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, size_in_bytes_token.get(),
        "Declared size in bytes " + std::to_string(size_bytes) +
            " does not match the combined size of the provided initial values, "
            "which is " +
            std::to_string(actual_size));
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandCreateBuffer>(
      std::move(start_token), std::move(result_identifier), values));
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

  std::unique_ptr<Token> program_identifier;
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
              program_identifier = std::move(token);
              return true;
            }},
           {Token::Type::kKeywordNumGroups,
            [this, &num_groups_x, &num_groups_y, &num_groups_z]() -> bool {
              for (auto* num_groups :
                   {&num_groups_x, &num_groups_y, &num_groups_z}) {
                auto maybe_num_groups = ParseUint32("number of groups");
                if (!maybe_num_groups.first) {
                  return false;
                }
                *num_groups = maybe_num_groups.second;
              }
              return true;
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandRunCompute>(
      std::move(start_token), std::move(program_identifier), num_groups_x,
      num_groups_y, num_groups_z));
  return true;
}

bool Parser::ParseCommandRunGraphics() {
  auto start_token = tokenizer_->NextToken();

  std::unique_ptr<Token> program_identifier;
  std::unordered_map<size_t, VertexAttributeInfo> vertex_data;
  std::unique_ptr<Token> index_data_buffer_identifier;
  size_t vertex_count;
  CommandRunGraphics::Topology topology;
  std::unordered_map<size_t, std::unique_ptr<Token>> framebuffer_attachments;

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
              program_identifier = std::move(token);
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
                vertex_data.insert({maybe_location.second,
                                    std::move(maybe_vertex_list.second)});
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
              index_data_buffer_identifier = std::move(token);
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
              auto square_brace_token = tokenizer_->NextToken();
              if (square_brace_token->GetText() != "[") {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, square_brace_token.get(),
                    "Expected '[' to commence start of "
                    "framebuffer attachments, got '" +
                        square_brace_token->GetText() + "'");
                return false;
              }
              std::unordered_map<size_t, std::unique_ptr<Token>>
                  observed_locations;
              std::unordered_map<std::string, std::unique_ptr<Token>>
                  observed_identifiers;
              while (tokenizer_->PeekNextToken()->GetText() != "]") {
                auto location_token = tokenizer_->PeekNextToken();
                auto maybe_location = ParseUint32("location");
                if (!maybe_location.first) {
                  return false;
                }
                if (observed_locations.count(maybe_location.second) > 0) {
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError, location_token.get(),
                      "Duplicate key: " +
                          std::to_string(maybe_location.second) +
                          " is already used as a key at " +
                          observed_locations.at(maybe_location.second)
                              ->GetLocationString());
                  return false;
                }
                observed_locations.emplace(maybe_location.second,
                                           std::move(location_token));
                auto arrow_token = tokenizer_->NextToken();
                if (arrow_token->GetText() != "->") {
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError, arrow_token.get(),
                      "Expected '->', got '" + arrow_token->GetText() + "'");
                  return false;
                }
                // We want two copies of this token, so we peek to get the first
                // one.
                auto identifier_token_for_map = tokenizer_->PeekNextToken();
                auto identifier_token = tokenizer_->NextToken();
                if (!identifier_token->IsIdentifier()) {
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError, identifier_token.get(),
                      "Expected identifier for framebuffer attachment, got '" +
                          identifier_token->GetText() + "'");
                  return false;
                }
                if (observed_identifiers.count(identifier_token->GetText()) >
                    0) {
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError, identifier_token.get(),
                      "Duplicate attachment: '" + identifier_token->GetText() +
                          "' is already attached at " +
                          observed_identifiers.at(identifier_token->GetText())
                              ->GetLocationString());
                  return false;
                }
                observed_identifiers.emplace(
                    identifier_token->GetText(),
                    std::move(identifier_token_for_map));

                framebuffer_attachments.insert(
                    {maybe_location.second, std::move(identifier_token)});
                auto comma_or_square_brace_token = tokenizer_->PeekNextToken();
                if (comma_or_square_brace_token->GetText() == ",") {
                  tokenizer_->NextToken();
                } else if (comma_or_square_brace_token->GetText() != "]") {
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError,
                      comma_or_square_brace_token.get(),
                      "Expected ',' or ']', got '" +
                          comma_or_square_brace_token->GetText() + "'");
                  return false;
                }
              }
              tokenizer_->NextToken();
              return true;
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandRunGraphics>(
      std::move(start_token), std::move(program_identifier),
      std::move(vertex_data), std::move(index_data_buffer_identifier),
      vertex_count, topology, std::move(framebuffer_attachments)));
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

  auto kind_keyword = tokenizer_->NextToken();
  if (kind_keyword->GetType() != Token::Type::kKeywordKind) {
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               kind_keyword.get(),
                               "Missing parameter 'KIND' to specify which kind "
                               "of shader this is, got '" +
                                   kind_keyword->GetText() + "'");
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

bool Parser::ParseCommandDumpBufferBinary() {
  auto start_token = tokenizer_->NextToken();
  std::unique_ptr<Token> buffer_identifier;
  std::unique_ptr<Token> filename;
  if (!ParseParameters(
          {{Token::Type::kKeywordBuffer,
            [this, &buffer_identifier]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(MessageConsumer::Severity::kError,
                                           token.get(),
                                           "Expected buffer identifier, got '" +
                                               token->GetText() + "'");
                return false;
              }
              buffer_identifier = std::move(token);
              return true;
            }},
           {Token::Type::kKeywordFile, [this, &filename]() -> bool {
              filename = tokenizer_->NextToken();
              if (!filename->IsString()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, filename.get(),
                    "Expected file to which to dump buffer, got '" +
                        filename->GetText() + "'");
                return false;
              }
              return true;
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandDumpBufferBinary>(
      std::move(start_token), std::move(buffer_identifier),
      std::move(filename)));
  return true;
}

bool Parser::ParseCommandDumpBufferText() {
  auto start_token = tokenizer_->NextToken();
  std::unique_ptr<Token> buffer_identifier;
  std::unique_ptr<Token> filename;
  std::vector<CommandDumpBufferText::FormatEntry> format_entries;
  if (!ParseParameters(
          {{Token::Type::kKeywordBuffer,
            [this, &buffer_identifier]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(MessageConsumer::Severity::kError,
                                           token.get(),
                                           "Expected buffer identifier, got '" +
                                               token->GetText() + "'");
                return false;
              }
              buffer_identifier = std::move(token);
              return true;
            }},
           {Token::Type::kKeywordFile,
            [this, &filename]() -> bool {
              filename = tokenizer_->NextToken();
              if (!filename->IsString()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, filename.get(),
                    "Expected file to which to dump buffer, got '" +
                        filename->GetText() + "'");
                return false;
              }
              return true;
            }},
           {Token::Type::kKeywordFormat, [this, &format_entries]() -> bool {
              while (true) {
                CommandDumpBufferText::FormatEntry::Kind kind;
                switch (tokenizer_->PeekNextToken()->GetType()) {
                  case Token::Type::kKeywordSkipBytes:
                    kind = CommandDumpBufferText::FormatEntry::Kind::kSkip;
                    break;
                  case Token::Type::kKeywordTypeByte:
                    kind = CommandDumpBufferText::FormatEntry::Kind::kByte;
                    break;
                  case Token::Type::kKeywordTypeFloat:
                    kind = CommandDumpBufferText::FormatEntry::Kind::kFloat;
                    break;
                  case Token::Type::kKeywordTypeInt:
                    kind = CommandDumpBufferText::FormatEntry::Kind::kInt;
                    break;
                  case Token::Type::kKeywordTypeUint: {
                    kind = CommandDumpBufferText::FormatEntry::Kind::kUint;
                    break;
                    case Token::Type::kString:
                      kind = CommandDumpBufferText::FormatEntry::Kind::kString;
                      break;
                    default:
                      return true;
                  }
                }
                auto format_start_token = tokenizer_->NextToken();
                size_t count;
                if (kind == CommandDumpBufferText::FormatEntry::Kind::kString) {
                  count = 0;
                } else {
                  auto maybe_count = ParseUint32("count");
                  if (!maybe_count.first) {
                    return false;
                  }
                  count = maybe_count.second;
                }
                format_entries.push_back(
                    {std::move(format_start_token), kind, count});
              }
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandDumpBufferText>(
      std::move(start_token), std::move(buffer_identifier), std::move(filename),
      std::move(format_entries)));
  return true;
}

bool Parser::ParseCommandDumpRenderbuffer() {
  auto start_token = tokenizer_->NextToken();
  std::unique_ptr<Token> renderbuffer_identifier;
  std::unique_ptr<Token> filename;
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
              renderbuffer_identifier = std::move(token);
              return true;
            }},
           {Token::Type::kKeywordFile, [this, &filename]() -> bool {
              filename = tokenizer_->NextToken();
              if (!filename->IsString()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, filename.get(),
                    "Expected file to which to dump renderbuffer, got '" +
                        filename->GetText() + "'");
                return false;
              }
              return true;
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandDumpRenderbuffer>(
      std::move(start_token), std::move(renderbuffer_identifier),
      std::move(filename)));
  return true;
}

bool Parser::ParseCommandSetSamplerParameter() {
  auto start_token = tokenizer_->NextToken();
  std::unique_ptr<Token> sampler_identifier;
  TextureParameter parameter;
  TextureParameterValue parameter_value;

  if (!ParseParameters(
          {{Token::Type::kKeywordSampler,
            [this, &sampler_identifier]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected identifier for target sampler, got '" +
                        token->GetText() + "'");
                return false;
              }
              sampler_identifier = std::move(token);
              return true;
            }},
           {Token::Type::kKeywordParameter,
            [this, &parameter]() -> bool {
              auto token = tokenizer_->NextToken();
              switch (token->GetType()) {
                case Token::Type::kKeywordTextureMagFilter:
                  parameter = TextureParameter::kMagFilter;
                  return true;
                case Token::Type::kKeywordTextureMinFilter:
                  parameter = TextureParameter::kMinFilter;
                  return true;
                default:
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError, token.get(),
                      "Unknown sampler parameter '" + token->GetText() + "'");
                  return false;
              }
            }},
           {Token::Type::kKeywordValue, [this, &parameter_value]() -> bool {
              auto token = tokenizer_->NextToken();
              switch (token->GetType()) {
                case Token::Type::kKeywordLinear:
                  parameter_value = TextureParameterValue::kLinear;
                  return true;
                case Token::Type::kKeywordNearest:
                  parameter_value = TextureParameterValue::kNearest;
                  return true;
                default:
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError, token.get(),
                      "Unknown sampler parameter value '" + token->GetText() +
                          "'");
                  return false;
              }
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandSetSamplerParameter>(
      std::move(start_token), std::move(sampler_identifier), parameter,
      parameter_value));
  return true;
}

bool Parser::ParseCommandSetTextureParameter() {
  auto start_token = tokenizer_->NextToken();
  std::unique_ptr<Token> texture_identifier;
  TextureParameter parameter;
  TextureParameterValue parameter_value;

  if (!ParseParameters(
          {{Token::Type::kKeywordTexture,
            [this, &texture_identifier]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsIdentifier()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected identifier for target texture, got '" +
                        token->GetText() + "'");
                return false;
              }
              texture_identifier = std::move(token);
              return true;
            }},
           {Token::Type::kKeywordParameter,
            [this, &parameter]() -> bool {
              auto token = tokenizer_->NextToken();
              switch (token->GetType()) {
                case Token::Type::kKeywordTextureMagFilter:
                  parameter = TextureParameter::kMagFilter;
                  return true;
                case Token::Type::kKeywordTextureMinFilter:
                  parameter = TextureParameter::kMinFilter;
                  return true;
                default:
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError, token.get(),
                      "Unknown texture parameter '" + token->GetText() + "'");
                  return false;
              }
            }},
           {Token::Type::kKeywordValue, [this, &parameter_value]() -> bool {
              auto token = tokenizer_->NextToken();
              switch (token->GetType()) {
                case Token::Type::kKeywordLinear:
                  parameter_value = TextureParameterValue::kLinear;
                  return true;
                case Token::Type::kKeywordNearest:
                  parameter_value = TextureParameterValue::kNearest;
                  return true;
                default:
                  message_consumer_->Message(
                      MessageConsumer::Severity::kError, token.get(),
                      "Unknown texture parameter value '" + token->GetText() +
                          "'");
                  return false;
              }
            }}})) {
    return false;
  }
  parsed_commands_.push_back(MakeUnique<CommandSetTextureParameter>(
      std::move(start_token), std::move(texture_identifier), parameter,
      parameter_value));
  return true;
}

bool Parser::ParseCommandSetUniform() {
  auto start_token = tokenizer_->NextToken();
  std::unique_ptr<Token> program_identifier;
  size_t location = 0;
  std::unique_ptr<Token> name = nullptr;
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
              program_identifier = std::move(token);
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
           {Token::Type::kKeywordName,
            [this, &name]() -> bool {
              auto token = tokenizer_->NextToken();
              if (!token->IsString()) {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Expected string for uniform name, got '" +
                        token->GetText() + "'");
                return false;
              }
              name = std::move(token);
              return true;
            }},
           {Token::Type::kKeywordType,
            [this, &maybe_array_size, &type]() -> bool {
              auto token = tokenizer_->NextToken();
              if (token->GetType() == Token::Type::kKeywordTypeFloat) {
                type = UniformValue::ElementType::kFloat;
              } else if (token->GetType() == Token::Type::kKeywordTypeVec2) {
                type = UniformValue::ElementType::kVec2;
              } else if (token->GetType() == Token::Type::kKeywordTypeVec3) {
                type = UniformValue::ElementType::kVec3;
              } else if (token->GetType() == Token::Type::kKeywordTypeVec4) {
                type = UniformValue::ElementType::kVec4;
              } else if (token->GetType() == Token::Type::kKeywordTypeInt) {
                type = UniformValue::ElementType::kInt;
              } else if (token->GetType() == Token::Type::kKeywordTypeIvec2) {
                type = UniformValue::ElementType::kIvec2;
              } else if (token->GetType() == Token::Type::kKeywordTypeIvec3) {
                type = UniformValue::ElementType::kIvec3;
              } else if (token->GetType() == Token::Type::kKeywordTypeIvec4) {
                type = UniformValue::ElementType::kIvec4;
              } else if (token->GetType() == Token::Type::kKeywordTypeUint) {
                type = UniformValue::ElementType::kUint;
              } else if (token->GetType() == Token::Type::kKeywordTypeUvec2) {
                type = UniformValue::ElementType::kUvec2;
              } else if (token->GetType() == Token::Type::kKeywordTypeUvec3) {
                type = UniformValue::ElementType::kUvec3;
              } else if (token->GetType() == Token::Type::kKeywordTypeUvec4) {
                type = UniformValue::ElementType::kUvec4;
              } else if (token->GetType() == Token::Type::kKeywordTypeMat2x2) {
                type = UniformValue::ElementType::kMat2x2;
              } else if (token->GetType() == Token::Type::kKeywordTypeMat2x3) {
                type = UniformValue::ElementType::kMat2x3;
              } else if (token->GetType() == Token::Type::kKeywordTypeMat2x4) {
                type = UniformValue::ElementType::kMat2x4;
              } else if (token->GetType() == Token::Type::kKeywordTypeMat3x2) {
                type = UniformValue::ElementType::kMat3x2;
              } else if (token->GetType() == Token::Type::kKeywordTypeMat3x3) {
                type = UniformValue::ElementType::kMat3x3;
              } else if (token->GetType() == Token::Type::kKeywordTypeMat3x4) {
                type = UniformValue::ElementType::kMat3x4;
              } else if (token->GetType() == Token::Type::kKeywordTypeMat4x2) {
                type = UniformValue::ElementType::kMat4x2;
              } else if (token->GetType() == Token::Type::kKeywordTypeMat4x3) {
                type = UniformValue::ElementType::kMat4x3;
              } else if (token->GetType() == Token::Type::kKeywordTypeMat4x4) {
                type = UniformValue::ElementType::kMat4x4;
              } else if (token->GetType() ==
                         Token::Type::kKeywordTypeSampler2d) {
                type = UniformValue::ElementType::kSampler2d;
              } else {
                message_consumer_->Message(
                    MessageConsumer::Severity::kError, token.get(),
                    "Unexpected type '" + token->GetText() + "'");
                return false;
              }
              if (tokenizer_->PeekNextToken()->GetText() == "[") {
                tokenizer_->NextToken();
                maybe_array_size = ParseUint32("array size");
                if (!maybe_array_size.first) {
                  return false;
                }
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
           {Token::Type::kKeywordValues,
            [this, &values]() -> bool {
              while (tokenizer_->PeekNextToken()->IsIntLiteral() ||
                     tokenizer_->PeekNextToken()->IsFloatLiteral()) {
                values.push_back(tokenizer_->NextToken());
              }
              return true;
            }}},
          // LOCATION and NAME are mutually exclusive
          {{Token::Type::kKeywordLocation, Token::Type::kKeywordName}}, {})) {
    return false;
  }
  auto maybe_uniform_value =
      ProcessUniformValue(type, maybe_array_size, values);
  if (!maybe_uniform_value.first) {
    return false;
  }
  if (name == nullptr) {
    // The uniform has been specified via a location
    parsed_commands_.push_back(MakeUnique<CommandSetUniform>(
        std::move(start_token), std::move(program_identifier), location,
        maybe_uniform_value.second));
  } else {
    // The uniform has been specified via a name
    parsed_commands_.push_back(MakeUnique<CommandSetUniform>(
        std::move(start_token), std::move(program_identifier), std::move(name),
        maybe_uniform_value.second));
  }
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
    case UniformValue::ElementType::kIvec4:
    case UniformValue::ElementType::kSampler2d: {
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
    const std::map<Token::Type, std::function<bool()>>& parameter_parsers,
    const std::map<Token::Type, Token::Type>& mutually_exclusive,
    const std::set<Token::Type>& optional_params) {
  // Check that any token types that are regarded as mutually exclusive do have
  // associated parser entries.
  for (const auto& entry : mutually_exclusive) {
    (void)entry;  // Keep release-mode compilers happy
    assert(parameter_parsers.count(entry.first) != 0 &&
           parameter_parsers.count(entry.second) != 0 &&
           "Mutual exclusion specified for parameter for which there is no "
           "parser");
  }

  std::map<Token::Type, std::unique_ptr<Token>> observed;
  while (true) {
    auto token = tokenizer_->PeekNextToken();
    auto token_type = token->GetType();
    if (parameter_parsers.count(token_type) == 0) {
      break;
    }
    if (observed.count(token_type) > 0) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, token.get(),
          "Duplicate parameter '" + token->GetText() + "'");
      return false;
    }
    observed.insert({token_type, std::move(token)});
    tokenizer_->NextToken();
    if (!parameter_parsers.at(token_type)()) {
      return false;
    }
  }

  bool found_errors = false;

  // This captures the parameters associated with mutually-exclusive pairs: we
  // record that we have handled them so that when we finally look for missing
  // parameters we do not consider them again.
  std::set<Token::Type> already_handled;
  for (const auto& pair : mutually_exclusive) {
    if (observed.count(pair.first) > 0 && observed.count(pair.second) > 0) {
      auto* first_token = observed.at(pair.first).get();
      auto* second_token = observed.at(pair.second).get();
      message_consumer_->Message(
          MessageConsumer::Severity::kError, first_token,
          "Parameters '" + first_token->GetText() + "' and '" +
              second_token->GetText() +
              "' are mutually exclusive; both are present at " +
              first_token->GetLocationString() + " and " +
              second_token->GetLocationString());
      found_errors = true;
    } else if (observed.count(pair.first) == 0 &&
               observed.count(pair.second) == 0) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, tokenizer_->PeekNextToken().get(),
          "Missing parameter '" + Tokenizer::KeywordToString(pair.first) +
              "' or '" + Tokenizer::KeywordToString(pair.second) + "'");
      found_errors = true;
    }
    already_handled.insert(pair.first);
    already_handled.insert(pair.second);
  }

  for (const auto& entry : parameter_parsers) {
    if (already_handled.count(entry.first) == 0 &&
        optional_params.count(entry.first) == 0 &&
        observed.count(entry.first) == 0) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, tokenizer_->PeekNextToken().get(),
          "Missing parameter '" + Tokenizer::KeywordToString(entry.first) +
              "'");
      found_errors = true;
    }
  }
  return !found_errors;
}

bool Parser::ParseParameters(
    const std::map<Token::Type, std::function<bool()>>& parameter_parsers) {
  return ParseParameters(parameter_parsers, {}, {});
}

std::pair<bool, VertexAttributeInfo> Parser::ParseVertexAttributeInfo() {
  std::unique_ptr<Token> buffer_identifier;
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
              buffer_identifier = std::move(token);
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
    return {false, VertexAttributeInfo(nullptr, 0, 0, 0)};
  }
  return {true, VertexAttributeInfo(std::move(buffer_identifier), offset_bytes,
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

std::pair<bool, ValuesSegment> Parser::ParseValuesSegment() {
  std::pair<bool, ValuesSegment> failure = {
      false, ValuesSegment(std::vector<uint8_t>())};
  auto token = tokenizer_->NextToken();
  switch (token->GetType()) {
    case Token::Type::kKeywordTypeByte: {
      std::vector<uint8_t> byte_data;
      while (tokenizer_->PeekNextToken()->GetType() ==
             Token::Type::kIntLiteral) {
        auto maybe_byte = ParseUint8("value");
        if (!maybe_byte.first) {
          return failure;
        }
        byte_data.push_back(maybe_byte.second);
      }
      if ((byte_data.size() % 4) != 0) {
        message_consumer_->Message(
            MessageConsumer::Severity::kError, token.get(),
            "The number of byte literals supplied in a buffer initializer must "
            "be a multiple of 4; found a sequence of " +
                std::to_string(byte_data.size()) + " literals");
        return failure;
      }
      return {true, ValuesSegment(byte_data)};
    }
    case Token::Type::kKeywordTypeFloat: {
      std::vector<float> float_data;
      while (tokenizer_->PeekNextToken()->GetType() ==
             Token::Type::kFloatLiteral) {
        auto maybe_float = ParseFloat("value");
        if (!maybe_float.first) {
          return failure;
        }
        float_data.push_back(maybe_float.second);
      }
      return {true, ValuesSegment(float_data)};
    }
    case Token::Type::kKeywordTypeInt: {
      std::vector<int32_t> int_data;
      while (tokenizer_->PeekNextToken()->GetType() ==
             Token::Type::kIntLiteral) {
        int_data.push_back(std::stoi(tokenizer_->NextToken()->GetText()));
      }
      return {true, ValuesSegment(int_data)};
    }
    default: {
      assert(token->GetType() == Token::Type::kKeywordTypeUint &&
             "Unexpected type for values segment.");
      std::vector<uint32_t> uint_data;
      while (tokenizer_->PeekNextToken()->GetType() ==
             Token::Type::kIntLiteral) {
        auto maybe_uint = ParseUint32("value");
        if (!maybe_uint.first) {
          return failure;
        }
        uint_data.push_back(maybe_uint.second);
      }
      return {true, ValuesSegment(uint_data)};
    }
  }
}

std::unique_ptr<ShaderTrapProgram> Parser::GetParsedProgram() {
  return MakeUnique<ShaderTrapProgram>(*api_version_,
                                       std::move(parsed_commands_));
}

}  // namespace shadertrap
