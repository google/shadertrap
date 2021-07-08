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

#ifndef LIBSHADERTRAP_TOKEN_H
#define LIBSHADERTRAP_TOKEN_H

#include <cstddef>
#include <string>

namespace shadertrap {

class Token {
 public:
  enum class Type {
    kEOS,
    kArrow,
    kComma,
    kFloatLiteral,
    kIdentifier,
    kIntLiteral,
    kKeywordAssertPixels,
    kKeywordAssertEqual,
    kKeywordAssertSimilarEmdHistogram,
    kKeywordBinding,
    kKeywordBindSampler,
    kKeywordBindShaderStorageBuffer,
    kKeywordBindTexture,
    kKeywordBindUniformBuffer,
    kKeywordBuffer,
    kKeywordBuffers,
    kKeywordCompileShader,
    kKeywordCompute,
    kKeywordCreateBuffer,
    kKeywordCreateEmptyTexture2d,
    kKeywordCreateProgram,
    kKeywordCreateRenderbuffer,
    kKeywordCreateSampler,
    kKeywordDeclareShader,
    kKeywordDimension,
    kKeywordDumpBufferBinary,
    kKeywordDumpBufferText,
    kKeywordDumpRenderbuffer,
    kKeywordEnd,
    kKeywordExpected,
    kKeywordFile,
    kKeywordFormat,
    kKeywordFragment,
    kKeywordFramebufferAttachments,
    kKeywordGl,
    kKeywordGles,
    kKeywordHeight,
    kKeywordIndexData,
    kKeywordInitType,
    kKeywordInitValues,
    kKeywordKind,
    kKeywordLinear,
    kKeywordLocation,
    kKeywordName,
    kKeywordNearest,
    kKeywordNumGroups,
    kKeywordOffsetBytes,
    kKeywordParameter,
    kKeywordProgram,
    kKeywordRectangle,
    kKeywordRenderbuffer,
    kKeywordRenderbuffers,
    kKeywordRunCompute,
    kKeywordRunGraphics,
    kKeywordSampler,
    kKeywordSetSamplerParameter,
    kKeywordSetTextureParameter,
    kKeywordSetUniform,
    kKeywordShader,
    kKeywordShaders,
    kKeywordSizeBytes,
    kKeywordSkipBytes,
    kKeywordStrideBytes,
    kKeywordTexture,
    kKeywordTextureMagFilter,
    kKeywordTextureMinFilter,
    kKeywordTextureUnit,
    kKeywordTolerance,
    kKeywordTopology,
    kKeywordTriangles,
    kKeywordType,
    kKeywordTypeByte,
    kKeywordTypeFloat,
    kKeywordTypeInt,
    kKeywordTypeIvec2,
    kKeywordTypeIvec3,
    kKeywordTypeIvec4,
    kKeywordTypeMat2x2,
    kKeywordTypeMat2x3,
    kKeywordTypeMat2x4,
    kKeywordTypeMat3x2,
    kKeywordTypeMat3x3,
    kKeywordTypeMat3x4,
    kKeywordTypeMat4x2,
    kKeywordTypeMat4x3,
    kKeywordTypeMat4x4,
    kKeywordTypeSampler2d,
    kKeywordTypeUint,
    kKeywordTypeUvec2,
    kKeywordTypeUvec3,
    kKeywordTypeUvec4,
    kKeywordTypeVec2,
    kKeywordTypeVec3,
    kKeywordTypeVec4,
    kKeywordValue,
    kKeywordValues,
    kKeywordVertex,
    kKeywordVertexCount,
    kKeywordVertexData,
    kKeywordWidth,
    kSquareBracketClose,
    kSquareBracketOpen,
    kString,
    kUnknown
  };

  explicit Token(Type type, size_t line, size_t column);

  Token(Type type, std::string text, size_t line, size_t column);

  const std::string& GetText() const { return text_; }

  Type GetType() const { return type_; }

  size_t GetLine() const { return line_; }

  size_t GetColumn() const { return column_; }

  bool IsEOS() const;

  bool IsIdentifier() const;

  bool IsIntLiteral() const;

  bool IsFloatLiteral() const;

  bool IsString() const;

  std::string GetLocationString() const;

 private:
  std::string text_;
  Type type_;
  size_t line_;
  size_t column_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_TOKEN_H
