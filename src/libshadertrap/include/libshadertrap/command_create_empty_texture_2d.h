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

#ifndef LIBSHADERTRAP_COMMAND_CREATE_EMPTY_TEXTURE_2D_H
#define LIBSHADERTRAP_COMMAND_CREATE_EMPTY_TEXTURE_2D_H

#include <cstdint>
#include <memory>
#include <string>

#include "libshadertrap/command.h"
#include "libshadertrap/token.h"

namespace shadertrap {

class CommandCreateEmptyTexture2D : public Command {
 public:
  CommandCreateEmptyTexture2D(std::unique_ptr<Token> start_token,
                              std::string result_identifier, uint32_t width,
                              uint32_t height);

  bool Accept(CommandVisitor* visitor) override;

  const std::string& GetResultIdentifier() const { return result_identifier_; }

  uint32_t GetWidth() const { return width_; }

  uint32_t GetHeight() const { return height_; }

 private:
  std::string result_identifier_;
  uint32_t width_;
  uint32_t height_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_CREATE_EMPTY_TEXTURE_2D_H
