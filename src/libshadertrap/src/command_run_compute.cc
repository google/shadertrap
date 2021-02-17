// Copyright 2021 The ShaderTrap Project Authors
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

#include "libshadertrap/command_run_compute.h"

#include <utility>

#include "libshadertrap/command_visitor.h"

namespace shadertrap {

CommandRunCompute::CommandRunCompute(std::unique_ptr<Token> start_token,
                                     std::unique_ptr<Token> program_identifier,
                                     size_t num_groups_x, size_t num_groups_y,
                                     size_t num_groups_z)
    : Command(std::move(start_token)),
      program_identifier_(std::move(program_identifier)),
      num_groups_x_(num_groups_x),
      num_groups_y_(num_groups_y),
      num_groups_z_(num_groups_z) {}

bool CommandRunCompute::Accept(CommandVisitor* visitor) {
  return visitor->VisitRunCompute(this);
}

}  // namespace shadertrap
