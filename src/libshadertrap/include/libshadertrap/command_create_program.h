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

#ifndef LIBSHADERTRAP_COMMAND_CREATE_PROGRAM_H
#define LIBSHADERTRAP_COMMAND_CREATE_PROGRAM_H

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "libshadertrap/command.h"
#include "libshadertrap/token.h"

namespace shadertrap {

class CommandCreateProgram : public Command {
 public:
  CommandCreateProgram(
      std::unique_ptr<Token> start_token,
      std::unique_ptr<Token> result_identifier,
      std::vector<std::unique_ptr<Token>> compiled_shader_identifier);

  bool Accept(CommandVisitor* visitor) override;

  const std::string& GetResultIdentifier() const {
    return result_identifier_->GetText();
  }

  const Token& GetResultIdentifierToken() const { return *result_identifier_; }

  const std::string& GetCompiledShaderIdentifier(size_t index) const {
    return GetCompiledShaderIdentifierToken(index).GetText();
  }

  const Token& GetCompiledShaderIdentifierToken(size_t index) const {
    assert(index < compiled_shader_identifiers_.size() &&
           "Index out of bounds.");
    return *compiled_shader_identifiers_.at(index);
  }

  size_t GetNumCompiledShaders() const {
    return compiled_shader_identifiers_.size();
  }

 private:
  std::unique_ptr<Token> result_identifier_;
  std::vector<std::unique_ptr<Token>> compiled_shader_identifiers_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_CREATE_PROGRAM_H
