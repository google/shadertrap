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

#ifndef LIBSHADERTRAP_COMMAND_DUMP_BUFFER_BINARY_H
#define LIBSHADERTRAP_COMMAND_DUMP_BUFFER_BINARY_H

#include <memory>
#include <string>

#include "libshadertrap/command.h"
#include "libshadertrap/token.h"

namespace shadertrap {

class CommandDumpBufferBinary : public Command {
 public:
  CommandDumpBufferBinary(std::unique_ptr<Token> start_token,
                          std::unique_ptr<Token> buffer_identifier,
                          std::unique_ptr<Token> filename);

  bool Accept(CommandVisitor* visitor) override;

  const std::string& GetBufferIdentifier() const {
    return buffer_identifier_->GetText();
  }

  const Token& GetBufferIdentifierToken() const { return *buffer_identifier_; }

  const std::string& GetFilename() const { return filename_->GetText(); }

  const Token& GetFilenameToken() const { return *filename_; }

 private:
  std::unique_ptr<Token> buffer_identifier_;
  std::unique_ptr<Token> filename_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_DUMP_BUFFER_BINARY_H
