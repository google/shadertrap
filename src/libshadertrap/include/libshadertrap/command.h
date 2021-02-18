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

#ifndef LIBSHADERTRAP_COMMAND_H
#define LIBSHADERTRAP_COMMAND_H

#include <memory>

#include "libshadertrap/token.h"

namespace shadertrap {

class CommandVisitor;

class Command {
 public:
  explicit Command(std::unique_ptr<Token> start_token);

  Command(const Command&) = delete;

  Command& operator=(const Command&) = delete;

  Command(Command&&) = delete;

  Command& operator=(Command&&) = delete;

  virtual ~Command();

  virtual bool Accept(CommandVisitor* visitor) = 0;

  const Token& GetStartToken() const { return *start_token_; }

 private:
  std::unique_ptr<Token> start_token_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_H
