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

#include "libshadertrap/command_assert_pixels.h"

#include <utility>

#include "libshadertrap/command_visitor.h"

namespace shadertrap {

CommandAssertPixels::CommandAssertPixels(
    std::unique_ptr<Token> start_token, uint8_t expected_r, uint8_t expected_g,
    uint8_t expected_b, uint8_t expected_a,
    std::unique_ptr<Token> renderbuffer_identifier, size_t rectangle_x,
    size_t rectangle_y, size_t rectangle_width, size_t rectangle_height,
    std::unique_ptr<Token> rectangle_width_token,
    std::unique_ptr<Token> rectangle_height_token)
    : Command(std::move(start_token)),
      expected_r_(expected_r),
      expected_g_(expected_g),
      expected_b_(expected_b),
      expected_a_(expected_a),
      renderbuffer_identifier_(std::move(renderbuffer_identifier)),
      rectangle_x_(rectangle_x),
      rectangle_y_(rectangle_y),
      rectangle_width_(rectangle_width),
      rectangle_height_(rectangle_height),
      rectangle_width_token_(std::move(rectangle_width_token)),
      rectangle_height_token_(std::move(rectangle_height_token)) {}

bool CommandAssertPixels::Accept(CommandVisitor* visitor) {
  return visitor->VisitAssertPixels(this);
}

}  // namespace shadertrap
