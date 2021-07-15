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

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "libshadertrap/command.h"
#include "libshadertrap/token.h"

namespace shadertrap {

class CommandAssertEqual : public Command {
 public:
  struct FormatEntry {
    enum class Kind { kByte, kFloat, kInt, kUint, kSkip };
    std::unique_ptr<Token> token;
    Kind kind;
    size_t count;
  };
  // Constructor used for an assertion about the equality of two buffers.
  CommandAssertEqual(std::unique_ptr<Token> start_token,
                     std::unique_ptr<Token> argument_identifier_1,
                     std::unique_ptr<Token> argument_identifier_2,
                     std::vector<FormatEntry> format_entries);

  // Constructor used for an assertion about the equality of two renderbuffers.
  CommandAssertEqual(std::unique_ptr<Token> start_token,
                     std::unique_ptr<Token> argument_identifier_1,
                     std::unique_ptr<Token> argument_identifier_2);

  bool Accept(CommandVisitor* visitor) override;

  bool GetArgumentsAreRenderbuffers() const {
    return arguments_are_renderbuffers_;
  }

  const std::string& GetArgumentIdentifier1() const {
    return argument_identifier_1_->GetText();
  }

  const Token& GetArgumentIdentifier1Token() const {
    return *argument_identifier_1_;
  }

  const std::string& GetArgumentIdentifier2() const {
    return argument_identifier_2_->GetText();
  }

  const Token& GetArgumentIdentifier2Token() const {
    return *argument_identifier_2_;
  }

  std::vector<FormatEntry>& GetFormatEntries() { return format_entries_; }

 private:
  // true if arguments are renderbuffers, false if arguments are buffers
  bool arguments_are_renderbuffers_;
  std::unique_ptr<Token> argument_identifier_1_;
  std::unique_ptr<Token> argument_identifier_2_;
  std::vector<FormatEntry> format_entries_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_ASSERT_EQUAL_H
