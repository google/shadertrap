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

#ifndef LIBSHADERTRAP_COMMAND_ASSERT_EQUAL_H
#define LIBSHADERTRAP_COMMAND_ASSERT_EQUAL_H

#include <memory>
#include <string>

#include "libshadertrap/command.h"
#include "libshadertrap/token.h"

namespace shadertrap {

class CommandAssertEqual : public Command {
 public:
  CommandAssertEqual(std::unique_ptr<Token> start_token,
                     std::unique_ptr<Token> buffer_identifier_1,
                     std::unique_ptr<Token> buffer_identifier_2);

  bool Accept(CommandVisitor* visitor) override;

  const std::string& GetBufferIdentifier1() const {
    return buffer_identifier_1_->GetText();
  }

  const Token* GetBufferIdentifier1Token() const {
    return buffer_identifier_1_.get();
  }

  const std::string& GetBufferIdentifier2() const {
    return buffer_identifier_2_->GetText();
  }

  const Token* GetBufferIdentifier2Token() const {
    return buffer_identifier_2_.get();
  }

 private:
  std::unique_ptr<Token> buffer_identifier_1_;
  std::unique_ptr<Token> buffer_identifier_2_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_ASSERT_EQUAL_H
