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

#include "libshadertrap/command_bind_texture.h"

#include <utility>

#include "libshadertrap/command_visitor.h"

namespace shadertrap {

CommandBindTexture::CommandBindTexture(
    std::unique_ptr<Token> start_token,
    std::unique_ptr<Token> texture_identifier, size_t texture_unit)
    : Command(std::move(start_token)),
      texture_identifier_(std::move(texture_identifier)),
      texture_unit_(texture_unit) {}

bool CommandBindTexture::Accept(CommandVisitor* visitor) {
  return visitor->VisitBindTexture(this);
}

}  // namespace shadertrap
