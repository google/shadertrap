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

#include "libshadertrap/token.h"

#include <sstream>
#include <utility>

namespace shadertrap {

Token::Token(Type type, size_t line, size_t column)
    : text_(std::string()), type_(type), line_(line), column_(column) {}

Token::Token(Type type, std::string text, size_t line, size_t column)
    : text_(std::move(text)), type_(type), line_(line), column_(column) {}

bool Token::IsEOS() const { return type_ == Type::kEOS; }

bool Token::IsIdentifier() const { return type_ == Type::kIdentifier; }

bool Token::IsIntLiteral() const { return type_ == Type::kIntLiteral; }

bool Token::IsFloatLiteral() const { return type_ == Type::kFloatLiteral; }

bool Token::IsString() const { return type_ == Type::kString; }

std::string Token::GetLocationString() const {
  std::stringstream stringstream;
  stringstream << GetLine() << ":" << GetColumn();
  return stringstream.str();
}

}  // namespace shadertrap
