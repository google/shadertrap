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

#include "libshadertrap/tokenizer.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <utility>

#include "libshadertrap/make_unique.h"

namespace shadertrap {

namespace {
const uint8_t kFf = 0x0c;
}  // namespace

Tokenizer::Tokenizer(std::string data) : data_(std::move(data)) {}

std::unique_ptr<Token> Tokenizer::NextToken(
    bool ignore_whitespace_and_comments) {
  if (ignore_whitespace_and_comments) {
    SkipWhitespaceAndComments();
  }
  const size_t start_line = line_;
  const size_t start_column = column_;
  if (position_ >= data_.length()) {
    return MakeUnique<Token>(Token::Type::kEOS, start_line, start_column);
  }
  if (data_[position_] == ',') {
    AdvanceCharacter();
    return MakeUnique<Token>(Token::Type::kComma, ",", start_line,
                             start_column);
  }

  if (data_[position_] == '[') {
    AdvanceCharacter();
    return MakeUnique<Token>(Token::Type::kSquareBracketOpen, "[", start_line,
                             start_column);
  }

  if (data_[position_] == ']') {
    AdvanceCharacter();
    return MakeUnique<Token>(Token::Type::kSquareBracketClose, "]", start_line,
                             start_column);
  }

  if (data_[position_] == '-' && position_ + 1 < data_.length() &&
      data_[position_ + 1] == '>') {
    AdvanceCharacter();
    AdvanceCharacter();
    return MakeUnique<Token>(Token::Type::kArrow, "->", start_line,
                             start_column);
  }

  if (std::isalpha(data_[position_]) != 0 || data_[position_] == '_') {
    std::stringstream stringstream;
    stringstream << data_[position_];
    AdvanceCharacter();
    while (std::isalnum(data_[position_]) != 0 || data_[position_] == '_') {
      stringstream << data_[position_];
      AdvanceCharacter();
    }
    return MakeUnique<Token>(keyword_to_token_type.count(stringstream.str()) > 0
                                 ? keyword_to_token_type.at(stringstream.str())
                                 : Token::Type::kIdentifier,
                             stringstream.str(), start_line, start_column);
  }
  if (std::isdigit(data_[position_]) != 0 || data_[position_] == '.' ||
      data_[position_] == '-') {
    bool is_float = data_[position_] == '.';
    std::stringstream stringstream;
    stringstream << data_[position_];
    AdvanceCharacter();
    while (std::isdigit(data_[position_]) != 0 || data_[position_] == '.') {
      is_float |= data_[position_] == '.';
      stringstream << data_[position_];
      AdvanceCharacter();
    }
    return MakeUnique<Token>(
        is_float ? Token::Type::kFloatLiteral : Token::Type::kIntLiteral,
        stringstream.str(), start_line, start_column);
  }
  if (data_[position_] == '"') {
    size_t backup_position = position_;
    size_t backup_column = column_;
    std::stringstream stringstream;
    AdvanceCharacter();
    bool last_character_was_escape = false;
    while (position_ < data_.length() && data_[position_] != '\n' &&
           (last_character_was_escape || data_[position_] != '"')) {
      if (last_character_was_escape) {
        switch (data_[position_]) {
          case 'n':
            stringstream << '\n';
            break;
          case 't':
            stringstream << '\t';
            break;
          case '\\':
            stringstream << '\\';
            break;
          case '"':
            stringstream << '"';
            break;
          default:
            AdvanceCharacter();
            assert(false && "Bad escape sequence");
            return MakeUnique<Token>(Token::Type::kUnknown, start_line,
                                     start_column);
        }
        last_character_was_escape = false;
      } else if (data_[position_] == '\\') {
        last_character_was_escape = true;
      } else {
        stringstream << data_[position_];
      }
      AdvanceCharacter();
    }
    if (data_[position_] == '"') {
      AdvanceCharacter();
      return MakeUnique<Token>(Token::Type::kString, stringstream.str(),
                               start_line, start_column);
    }
    position_ = backup_position;
    column_ = backup_column;
  }
  return MakeUnique<Token>(Token::Type::kUnknown, start_line, start_column);
}

std::unique_ptr<Token> Tokenizer::NextToken() { return NextToken(true); }

std::unique_ptr<Token> Tokenizer::PeekNextToken(
    bool ignore_whitespace_and_comments) {
  size_t position_backup = position_;
  size_t line_backup = line_;
  size_t column_backup = column_;
  auto result = NextToken(ignore_whitespace_and_comments);
  position_ = position_backup;
  line_ = line_backup;
  column_ = column_backup;
  return result;
}

std::unique_ptr<Token> Tokenizer::PeekNextToken() {
  return PeekNextToken(true);
}

void Tokenizer::SkipWhitespace() {
  while (position_ < data_.size()) {
    switch (data_[position_]) {
      case '\0':
      case '\t':
      case '\r':
      case kFf:
      case ' ':
      case '\n':
        AdvanceCharacter();
        break;
      default:
        return;
    }
  }
}

void Tokenizer::SkipWhitespaceAndComments() {
  SkipWhitespace();
  while (position_ < data_.size() && data_[position_] == '#') {
    SkipLine();
    SkipWhitespace();
  }
}

std::unique_ptr<Token> Tokenizer::SkipSingleLineOfWhitespaceAndComments() {
  // Skip any whitespace, with the exception of '\n'
  bool found_newline_or_non_whitespace = false;
  while (position_ < data_.size() && !found_newline_or_non_whitespace) {
    switch (data_[position_]) {
      case '\0':
      case '\t':
      case '\r':
      case kFf:
      case ' ':
        AdvanceCharacter();
        break;
      default:
        found_newline_or_non_whitespace = true;
        break;
    }
  }
  if (position_ < data_.size()) {
    if (data_[position_] == '#') {
      // The rest of the line is a comment, so skip over it, returning the
      // content of the comment as a string token.
      // These local variables are needed because SkipLine() will update
      // |line_| and |column_|.
      size_t line = line_;
      size_t column = column_;
      return MakeUnique<Token>(Token::Type::kString, SkipLine(), line, column);
    }
    if (data_[position_] == '\n') {
      // We have hit the end of the line, so advance to the next line.
      AdvanceCharacter();
    }
  }
  return MakeUnique<Token>(Token::Type::kString, line_, column_);
}

void Tokenizer::AdvanceCharacter() {
  if (data_[position_] == '\n') {
    line_++;
    column_ = 1;
  } else {
    column_++;
  }
  position_++;
}

std::string Tokenizer::SkipLine() {
  std::stringstream stringstream;
  while (position_ < data_.length()) {
    stringstream << data_[position_];
    bool end_of_line = data_[position_] == '\n';
    AdvanceCharacter();
    if (end_of_line) {
      break;
    }
  }
  return stringstream.str();
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
// TODO(afd): Non constinit global variables are strongly discouraged.  Consider
//  an alternative approach.
const std::unordered_map<std::string, Token::Type>
    Tokenizer::keyword_to_token_type = {  // NOLINT(cert-err58-cpp)
        {"ASSERT_PIXELS", Token::Type::kKeywordAssertPixels},
        {"ASSERT_EQUAL", Token::Type::kKeywordAssertEqual},
        {"ASSERT_SIMILAR_EMD_HISTOGRAM",
         Token::Type::kKeywordAssertSimilarEmdHistogram},
        {"BINDING", Token::Type::kKeywordBinding},
        {"BIND_SAMPLER", Token::Type::kKeywordBindSampler},
        {"BIND_SHADER_STORAGE_BUFFER",
         Token::Type::kKeywordBindShaderStorageBuffer},
        {"BIND_TEXTURE", Token::Type::kKeywordBindTexture},
        {"BIND_UNIFORM_BUFFER", Token::Type::kKeywordBindUniformBuffer},
        {"BUFFER", Token::Type::kKeywordBuffer},
        {"BUFFERS", Token::Type::kKeywordBuffers},
        {"COMPILE_SHADER", Token::Type::kKeywordCompileShader},
        {"COMPUTE", Token::Type::kKeywordCompute},
        {"CREATE_BUFFER", Token::Type::kKeywordCreateBuffer},
        {"CREATE_EMPTY_TEXTURE_2D", Token::Type::kKeywordCreateEmptyTexture2d},
        {"CREATE_PROGRAM", Token::Type::kKeywordCreateProgram},
        {"CREATE_RENDERBUFFER", Token::Type::kKeywordCreateRenderbuffer},
        {"CREATE_SAMPLER", Token::Type::kKeywordCreateSampler},
        {"DECLARE_SHADER", Token::Type::kKeywordDeclareShader},
        {"DIMENSION", Token::Type::kKeywordDimension},
        {"DUMP_BUFFER_BINARY", Token::Type::kKeywordDumpBufferBinary},
        {"DUMP_BUFFER_TEXT", Token::Type::kKeywordDumpBufferText},
        {"DUMP_RENDERBUFFER", Token::Type::kKeywordDumpRenderbuffer},
        {"END", Token::Type::kKeywordEnd},
        {"EXPECTED", Token::Type::kKeywordExpected},
        {"FILE", Token::Type::kKeywordFile},
        {"FORMAT", Token::Type::kKeywordFormat},
        {"FRAGMENT", Token::Type::kKeywordFragment},
        {"FRAMEBUFFER_ATTACHMENTS",
         Token::Type::kKeywordFramebufferAttachments},
        {"GL", Token::Type::kKeywordGl},
        {"GLES", Token::Type::kKeywordGles},
        {"HEIGHT", Token::Type::kKeywordHeight},
        {"INDEX_DATA", Token::Type::kKeywordIndexData},
        {"INIT_TYPE", Token::Type::kKeywordInitType},
        {"INIT_VALUES", Token::Type::kKeywordInitValues},
        {"KIND", Token::Type::kKeywordKind},
        {"LINEAR", Token::Type::kKeywordLinear},
        {"LOCATION", Token::Type::kKeywordLocation},
        {"NAME", Token::Type::kKeywordName},
        {"NEAREST", Token::Type::kKeywordNearest},
        {"NUM_GROUPS", Token::Type::kKeywordNumGroups},
        {"OFFSET_BYTES", Token::Type::kKeywordOffsetBytes},
        {"PARAMETER", Token::Type::kKeywordParameter},
        {"PROGRAM", Token::Type::kKeywordProgram},
        {"RECTANGLE", Token::Type::kKeywordRectangle},
        {"RENDERBUFFER", Token::Type::kKeywordRenderbuffer},
        {"RENDERBUFFERS", Token::Type::kKeywordRenderbuffers},
        {"RUN_COMPUTE", Token::Type::kKeywordRunCompute},
        {"RUN_GRAPHICS", Token::Type::kKeywordRunGraphics},
        {"SAMPLER", Token::Type::kKeywordSampler},
        {"SET_SAMPLER_PARAMETER", Token::Type::kKeywordSetSamplerParameter},
        {"SET_TEXTURE_PARAMETER", Token::Type::kKeywordSetTextureParameter},
        {"SET_UNIFORM", Token::Type::kKeywordSetUniform},
        {"SHADER", Token::Type::kKeywordShader},
        {"SHADERS", Token::Type::kKeywordShaders},
        {"SIZE_BYTES", Token::Type::kKeywordSizeBytes},
        {"SKIP_BYTES", Token::Type::kKeywordSkipBytes},
        {"STRIDE_BYTES", Token::Type::kKeywordStrideBytes},
        {"TEXTURE", Token::Type::kKeywordTexture},
        {"TEXTURE_MAG_FILTER", Token::Type::kKeywordTextureMagFilter},
        {"TEXTURE_MIN_FILTER", Token::Type::kKeywordTextureMinFilter},
        {"TEXTURE_UNIT", Token::Type::kKeywordTextureUnit},
        {"TOLERANCE", Token::Type::kKeywordTolerance},
        {"TOPOLOGY", Token::Type::kKeywordTopology},
        {"TRIANGLES", Token::Type::kKeywordTriangles},
        {"TYPE", Token::Type::kKeywordType},
        {"byte", Token::Type::kKeywordTypeByte},
        {"float", Token::Type::kKeywordTypeFloat},
        {"int", Token::Type::kKeywordTypeInt},
        {"ivec2", Token::Type::kKeywordTypeIvec2},
        {"ivec3", Token::Type::kKeywordTypeIvec3},
        {"ivec4", Token::Type::kKeywordTypeIvec4},
        {"mat2x2", Token::Type::kKeywordTypeMat2x2},
        {"mat2x3", Token::Type::kKeywordTypeMat2x3},
        {"mat2x4", Token::Type::kKeywordTypeMat2x4},
        {"mat3x2", Token::Type::kKeywordTypeMat3x2},
        {"mat3x3", Token::Type::kKeywordTypeMat3x3},
        {"mat3x4", Token::Type::kKeywordTypeMat3x4},
        {"mat4x2", Token::Type::kKeywordTypeMat4x2},
        {"mat4x3", Token::Type::kKeywordTypeMat4x3},
        {"mat4x4", Token::Type::kKeywordTypeMat4x4},
        {"sampler2D", Token::Type::kKeywordTypeSampler2d},
        {"uint", Token::Type::kKeywordTypeUint},
        {"uvec2", Token::Type::kKeywordTypeUvec2},
        {"uvec3", Token::Type::kKeywordTypeUvec3},
        {"uvec4", Token::Type::kKeywordTypeUvec4},
        {"vec2", Token::Type::kKeywordTypeVec2},
        {"vec3", Token::Type::kKeywordTypeVec3},
        {"vec4", Token::Type::kKeywordTypeVec4},
        {"VALUE", Token::Type::kKeywordValue},
        {"VALUES", Token::Type::kKeywordValues},
        {"VERTEX", Token::Type::kKeywordVertex},
        {"VERTEX_COUNT", Token::Type::kKeywordVertexCount},
        {"VERTEX_DATA", Token::Type::kKeywordVertexData},
        {"WIDTH", Token::Type::kKeywordWidth}};
#pragma clang diagnostic pop

std::string Tokenizer::KeywordToString(Token::Type keyword_token_type) {
  auto maybe_result = std::find_if(
      keyword_to_token_type.begin(), keyword_to_token_type.end(),
      [keyword_token_type](const std::pair<std::string, Token::Type>& entry)
          -> bool { return entry.second == keyword_token_type; });
  assert(maybe_result != keyword_to_token_type.end() &&
         "A keyword must exist for every keyword token type.");
  return maybe_result->first;
}

}  // namespace shadertrap
