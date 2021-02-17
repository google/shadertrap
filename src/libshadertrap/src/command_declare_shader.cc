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

#include "libshadertrap/command_declare_shader.h"

#include <utility>

#include "libshadertrap/command_visitor.h"

namespace shadertrap {

CommandDeclareShader::CommandDeclareShader(
    std::unique_ptr<Token> start_token,
    std::unique_ptr<Token> result_identifier, Kind kind,
    std::string shader_text, size_t shader_start_line)
    : Command(std::move(start_token)),
      result_identifier_(std::move(result_identifier)),
      kind_(kind),
      shader_text_(std::move(shader_text)),
      shader_start_line_(shader_start_line) {}

bool CommandDeclareShader::Accept(CommandVisitor* visitor) {
  return visitor->VisitDeclareShader(this);
}

}  // namespace shadertrap
