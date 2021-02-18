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

#ifndef LIBSHADERTRAP_COMMAND_DUMP_RENDERBUFFER_H
#define LIBSHADERTRAP_COMMAND_DUMP_RENDERBUFFER_H

#include <memory>
#include <string>

#include "libshadertrap/command.h"
#include "libshadertrap/token.h"

namespace shadertrap {

class CommandDumpRenderbuffer : public Command {
 public:
  CommandDumpRenderbuffer(std::unique_ptr<Token> start_token,
                          std::unique_ptr<Token> renderbuffer_identifier,
                          std::string filename);

  bool Accept(CommandVisitor* visitor) override;

  const std::string& GetRenderbufferIdentifier() const {
    return renderbuffer_identifier_->GetText();
  }

  const Token* GetRenderbufferIdentifierToken() const {
    return renderbuffer_identifier_.get();
  }

  const std::string& GetFilename() const { return filename_; }

 private:
  std::unique_ptr<Token> renderbuffer_identifier_;
  std::string filename_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_DUMP_RENDERBUFFER_H
