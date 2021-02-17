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

#include "libshadertrap/command_run_graphics.h"

#include <utility>

#include "libshadertrap/command_visitor.h"

namespace shadertrap {

CommandRunGraphics::CommandRunGraphics(
    std::unique_ptr<Token> start_token,
    std::unique_ptr<Token> program_identifier,
    std::unordered_map<size_t, VertexAttributeInfo> vertex_data,
    std::unique_ptr<Token> index_data_buffer_identifier, size_t vertex_count,
    Topology topology,
    std::unordered_map<size_t, std::unique_ptr<Token>> framebuffer_attachments)
    : Command(std::move(start_token)),
      program_identifier_(std::move(program_identifier)),
      vertex_data_(std::move(vertex_data)),
      index_data_buffer_identifier_(std::move(index_data_buffer_identifier)),
      vertex_count_(vertex_count),
      topology_(topology),
      framebuffer_attachments_(std::move(framebuffer_attachments)) {}

bool CommandRunGraphics::Accept(CommandVisitor* visitor) {
  return visitor->VisitRunGraphics(this);
}

}  // namespace shadertrap
