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

#include "libshadertrap/command_assert_similar_emd_histogram.h"

#include <utility>

#include "libshadertrap/command_visitor.h"

namespace shadertrap {

CommandAssertSimilarEmdHistogram::CommandAssertSimilarEmdHistogram(
    std::unique_ptr<Token> start_token,
    std::unique_ptr<Token> renderbuffer_identifier_1,
    std::unique_ptr<Token> renderbuffer_identifier_2, float tolerance)
    : Command(std::move(start_token)),
      renderbuffer_identifier_1_(std::move(renderbuffer_identifier_1)),
      renderbuffer_identifier_2_(std::move(renderbuffer_identifier_2)),
      tolerance_(tolerance) {}

bool CommandAssertSimilarEmdHistogram::Accept(CommandVisitor* visitor) {
  return visitor->VisitAssertSimilarEmdHistogram(this);
}

}  // namespace shadertrap
