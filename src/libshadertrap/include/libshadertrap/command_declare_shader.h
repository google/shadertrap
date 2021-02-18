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

#ifndef LIBSHADERTRAP_COMMAND_DECLARE_SHADER_H
#define LIBSHADERTRAP_COMMAND_DECLARE_SHADER_H

#include <cstddef>
#include <memory>
#include <string>

#include "libshadertrap/command.h"
#include "libshadertrap/token.h"

namespace shadertrap {

class CommandDeclareShader : public Command {
 public:
  enum class Kind { VERTEX, FRAGMENT, COMPUTE };

  CommandDeclareShader(std::unique_ptr<Token> start_token,
                       std::unique_ptr<Token> result_identifier, Kind kind,
                       std::string shader_text, size_t shader_start_line);

  bool Accept(CommandVisitor* visitor) override;

  const std::string& GetResultIdentifier() const {
    return result_identifier_->GetText();
  }

  const Token& GetResultIdentifierToken() const { return *result_identifier_; }

  const std::string& GetShaderText() const { return shader_text_; }

  Kind GetKind() const { return kind_; }

  size_t GetShaderStartLine() const { return shader_start_line_; }

 private:
  std::unique_ptr<Token> result_identifier_;
  Kind kind_;
  std::string shader_text_;
  size_t shader_start_line_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_DECLARE_SHADER_H
