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

#ifndef LIBSHADERTRAP_COMMAND_SET_UNIFORM_H
#define LIBSHADERTRAP_COMMAND_SET_UNIFORM_H

#include <cstddef>
#include <memory>
#include <string>

#include "libshadertrap/command.h"
#include "libshadertrap/token.h"
#include "libshadertrap/uniform_value.h"

namespace shadertrap {

class CommandSetUniform : public Command {
 public:
  CommandSetUniform(std::unique_ptr<Token> start_token,
                    std::unique_ptr<Token> program_identifier, size_t location,
                    UniformValue value);

  bool Accept(CommandVisitor* visitor) override;

  const std::string& GetProgramIdentifier() const {
    return program_identifier_->GetText();
  }

  const Token& GetProgramIdentifierToken() const {
    return *program_identifier_;
  }

  size_t GetLocation() const { return location_; }

  const UniformValue& GetValue() const { return value_; }

 private:
  std::unique_ptr<Token> program_identifier_;
  size_t location_;
  UniformValue value_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_SET_UNIFORM_H
