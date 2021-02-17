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

#ifndef LIBSHADERTRAP_TOKENIZER_H
#define LIBSHADERTRAP_TOKENIZER_H

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include "libshadertrap/token.h"

namespace shadertrap {

class Tokenizer {
 public:
  explicit Tokenizer(std::string data);

  std::unique_ptr<Token> PeekNextToken();

  std::unique_ptr<Token> NextToken();

  std::unique_ptr<Token> PeekNextToken(bool ignore_whitespace_and_comments);

  std::unique_ptr<Token> NextToken(bool ignore_whitespace_and_comments);

  void SkipWhitespace();

  // Skips up to a single line of whitespace and comments. If a comment is
  // skipped then a string token capturing the content of the comment is
  // returned. Otherwise an empty string token is returned.
  std::unique_ptr<Token> SkipSingleLineOfWhitespaceAndComments();

  // Skips over a line, returning the line that was skipped.
  std::string SkipLine();

  size_t GetLine() const { return line_; }

  static std::string KeywordToString(Token::Type keyword_token_type);

 private:
  void AdvanceCharacter();

  void SkipWhitespaceAndComments();

  std::string data_;
  size_t position_ = 0;
  size_t line_ = 1;
  size_t column_ = 1;

  static const std::unordered_map<std::string, Token::Type>
      keyword_to_token_type;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_TOKENIZER_H
