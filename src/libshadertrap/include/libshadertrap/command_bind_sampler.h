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

#ifndef LIBSHADERTRAP_COMMAND_BIND_SAMPLER_H
#define LIBSHADERTRAP_COMMAND_BIND_SAMPLER_H

#include <cstddef>
#include <memory>
#include <string>

#include "libshadertrap/command.h"
#include "libshadertrap/token.h"

namespace shadertrap {

class CommandBindSampler : public Command {
 public:
  CommandBindSampler(std::unique_ptr<Token> start_token,
                     std::unique_ptr<Token> sampler_identifier,
                     size_t texture_unit_);

  bool Accept(CommandVisitor* visitor) override;

  const std::string& GetSamplerIdentifier() const {
    return sampler_identifier_->GetText();
  }

  const Token& GetSamplerIdentifierToken() const {
    return *sampler_identifier_;
  }

  size_t GetTextureUnit() const { return texture_unit_; }

 private:
  std::unique_ptr<Token> sampler_identifier_;
  size_t texture_unit_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_BIND_SAMPLER_H
