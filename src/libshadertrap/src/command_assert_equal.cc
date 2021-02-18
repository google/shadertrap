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

#include "libshadertrap/command_assert_equal.h"

#include <utility>

#include "libshadertrap/command_visitor.h"

namespace shadertrap {

CommandAssertEqual::CommandAssertEqual(
    std::unique_ptr<Token> start_token,
    std::unique_ptr<Token> buffer_identifier_1,
    std::unique_ptr<Token> buffer_identifier_2)
    : Command(std::move(start_token)),
      buffer_identifier_1_(std::move(buffer_identifier_1)),
      buffer_identifier_2_(std::move(buffer_identifier_2)) {}

bool CommandAssertEqual::Accept(CommandVisitor* visitor) {
  return visitor->VisitAssertEqual(this);
}

}  // namespace shadertrap
