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

#include "libshadertrap/command_create_program.h"

#include <utility>

#include "libshadertrap/command_visitor.h"

namespace shadertrap {

CommandCreateProgram::CommandCreateProgram(
    std::unique_ptr<Token> start_token,
    std::unique_ptr<Token> result_identifier,
    std::vector<std::unique_ptr<Token>> compiled_shader_identifiers)
    : Command(std::move(start_token)),
      result_identifier_(std::move(result_identifier)),
      compiled_shader_identifiers_(std::move(compiled_shader_identifiers)) {
  assert(!compiled_shader_identifiers_.empty() &&
         "At least one compiled shader identifier should be provided.");
}

bool CommandCreateProgram::Accept(CommandVisitor* visitor) {
  return visitor->VisitCreateProgram(this);
}

}  // namespace shadertrap
