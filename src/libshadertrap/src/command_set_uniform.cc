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

#include "libshadertrap/command_set_uniform.h"

#include <utility>

#include "libshadertrap/command_visitor.h"

namespace shadertrap {

CommandSetUniform::CommandSetUniform(std::unique_ptr<Token> start_token,
                                     std::unique_ptr<Token> program_identifier,
                                     size_t location, UniformValue value)
    : Command(std::move(start_token)),
      program_identifier_(std::move(program_identifier)),
      location_(location),
      name_(nullptr),
      value_(std::move(value)) {}

CommandSetUniform::CommandSetUniform(std::unique_ptr<Token> start_token,
                                     std::unique_ptr<Token> program_identifier,
                                     std::unique_ptr<Token> name,
                                     UniformValue value)
    : Command(std::move(start_token)),
      program_identifier_(std::move(program_identifier)),
      location_(0),
      name_(std::move(name)),
      value_(std::move(value)) {}

bool CommandSetUniform::Accept(CommandVisitor* visitor) {
  return visitor->VisitSetUniform(this);
}

}  // namespace shadertrap
