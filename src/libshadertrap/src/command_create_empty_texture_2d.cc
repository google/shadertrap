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

#include "libshadertrap/command_create_empty_texture_2d.h"

#include <utility>

#include "libshadertrap/command_visitor.h"

namespace shadertrap {

CommandCreateEmptyTexture2D::CommandCreateEmptyTexture2D(
    std::unique_ptr<Token> start_token,
    std::unique_ptr<Token> result_identifier, size_t width, size_t height)
    : Command(std::move(start_token)),
      result_identifier_(std::move(result_identifier)),
      width_(width),
      height_(height) {}

bool CommandCreateEmptyTexture2D::Accept(CommandVisitor* visitor) {
  return visitor->VisitCreateEmptyTexture2D(this);
}

}  // namespace shadertrap
