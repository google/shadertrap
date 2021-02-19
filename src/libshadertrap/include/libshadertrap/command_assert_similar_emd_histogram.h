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

#ifndef LIBSHADERTRAP_COMMAND_ASSERT_SIMILAR_EMD_HISTOGRAM_H
#define LIBSHADERTRAP_COMMAND_ASSERT_SIMILAR_EMD_HISTOGRAM_H

#include <memory>
#include <string>

#include "libshadertrap/command.h"
#include "libshadertrap/token.h"

namespace shadertrap {

class CommandAssertSimilarEmdHistogram : public Command {
 public:
  CommandAssertSimilarEmdHistogram(
      std::unique_ptr<Token> start_token,
      std::unique_ptr<Token> renderbuffer_identifier_1,
      std::unique_ptr<Token> renderbuffer_identifier_2, float tolerance);

  bool Accept(CommandVisitor* visitor) override;

  const std::string& GetRenderbufferIdentifier1() const {
    return renderbuffer_identifier_1_->GetText();
  }

  const Token& GetRenderbufferIdentifier1Token() const {
    return *renderbuffer_identifier_1_;
  }

  const std::string& GetRenderbufferIdentifier2() const {
    return renderbuffer_identifier_2_->GetText();
  }

  const Token& GetRenderbufferIdentifier2Token() const {
    return *renderbuffer_identifier_2_;
  }

  float GetTolerance() const { return tolerance_; }

 private:
  std::unique_ptr<Token> renderbuffer_identifier_1_;
  std::unique_ptr<Token> renderbuffer_identifier_2_;
  float tolerance_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_ASSERT_SIMILAR_EMD_HISTOGRAM_H
