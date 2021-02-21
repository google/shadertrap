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

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>

#include "libshadertrap/command.h"
#include "libshadertrap/token.h"
#include "libshadertrap/uniform_value.h"

namespace shadertrap {

class CommandSetUniform : public Command {
 public:
  // Constructor for setting a uniform by location
  CommandSetUniform(std::unique_ptr<Token> start_token,
                    std::unique_ptr<Token> program_identifier, size_t location,
                    UniformValue value);

  // Constructor for setting a uniform by name
  CommandSetUniform(std::unique_ptr<Token> start_token,
                    std::unique_ptr<Token> program_identifier,
                    std::unique_ptr<Token> name, UniformValue value);

  bool Accept(CommandVisitor* visitor) override;

  const std::string& GetProgramIdentifier() const {
    return program_identifier_->GetText();
  }

  const Token& GetProgramIdentifierToken() const {
    return *program_identifier_;
  }

  bool HasLocation() const { return name_ == nullptr; }

  bool HasName() const { return name_ != nullptr; }

  size_t GetLocation() const {
    assert(HasLocation() && "Uniform not identified via a location");
    return location_;
  }

  const std::string& GetName() const {
    assert(HasName() && "Uniform not identified via a name");
    return name_->GetText();
  }

  const Token& GetNameToken() const {
    assert(HasName() && "Uniform not identified via a name");
    return *name_;
  }

  const UniformValue& GetValue() const { return value_; }

 private:
  std::unique_ptr<Token> program_identifier_;
  // If |name_| == nullptr then this uniform is identified via |location_|.
  // Otherwise, |location_| is ignored and the uniform is identified via
  // |name_|.
  size_t location_;
  std::unique_ptr<Token> name_;
  UniformValue value_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_SET_UNIFORM_H
