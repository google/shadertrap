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

#ifndef LIBSHADERTRAP_COMMAND_RUN_GRAPHICS_H
#define LIBSHADERTRAP_COMMAND_RUN_GRAPHICS_H

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include "libshadertrap/command.h"
#include "libshadertrap/token.h"
#include "libshadertrap/vertex_attribute_info.h"

namespace shadertrap {

class CommandRunGraphics : public Command {
 public:
  enum class Topology { kTriangles };

  CommandRunGraphics(
      std::unique_ptr<Token> start_token,
      std::unique_ptr<Token> program_identifier,
      std::unordered_map<size_t, VertexAttributeInfo> vertex_data,
      std::unique_ptr<Token> index_data_buffer_identifier, size_t vertex_count,
      Topology topology,
      std::unordered_map<size_t, std::unique_ptr<Token>>
          framebuffer_attachments);

  bool Accept(CommandVisitor* visitor) override;

  const std::string& GetProgramIdentifier() const {
    return program_identifier_->GetText();
  }

  const Token& GetProgramIdentifierToken() const {
    return *program_identifier_;
  }

  const std::unordered_map<size_t, VertexAttributeInfo>& GetVertexData() const {
    return vertex_data_;
  }

  const std::string& GetIndexDataBufferIdentifier() const {
    return index_data_buffer_identifier_->GetText();
  }

  const Token& GetIndexDataBufferIdentifierToken() const {
    return *index_data_buffer_identifier_;
  }

  size_t GetVertexCount() const { return vertex_count_; }

  Topology GetTopology() const { return topology_; }

  const std::unordered_map<size_t, std::unique_ptr<Token>>&
  GetFramebufferAttachments() const {
    return framebuffer_attachments_;
  }

 private:
  std::unique_ptr<Token> program_identifier_;
  std::unordered_map<size_t, VertexAttributeInfo> vertex_data_;
  std::unique_ptr<Token> index_data_buffer_identifier_;
  size_t vertex_count_;
  Topology topology_;
  std::unordered_map<size_t, std::unique_ptr<Token>> framebuffer_attachments_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_RUN_GRAPHICS_H
