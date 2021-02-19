// Copyright 2021 The ShaderTrap Project Authors
// r
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

#ifndef LIBSHADERTRAP_COMMAND_SET_TEXTURE_PARAMETER_H
#define LIBSHADERTRAP_COMMAND_SET_TEXTURE_PARAMETER_H

#include <memory>
#include <string>

#include "libshadertrap/command.h"
#include "libshadertrap/texture_parameter.h"
#include "libshadertrap/token.h"

namespace shadertrap {

class CommandSetTextureParameter : public Command {
 public:
  CommandSetTextureParameter(std::unique_ptr<Token> start_token,
                             std::unique_ptr<Token> texture_identifier,
                             TextureParameter parameter,
                             TextureParameterValue parameter_value);

  bool Accept(CommandVisitor* visitor) override;

  const std::string& GetTextureIdentifier() const {
    return texture_identifier_->GetText();
  }

  const Token& GetTextureIdentifierToken() const {
    return *texture_identifier_;
  }

  TextureParameter GetParameter() const { return parameter_; }

  TextureParameterValue GetParameterValue() const { return parameter_value_; }

 private:
  std::unique_ptr<Token> texture_identifier_;
  TextureParameter parameter_;
  TextureParameterValue parameter_value_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_SET_TEXTURE_PARAMETER_H
