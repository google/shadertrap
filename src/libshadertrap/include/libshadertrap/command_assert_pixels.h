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

#ifndef LIBSHADERTRAP_COMMAND_ASSERT_PIXELS_H
#define LIBSHADERTRAP_COMMAND_ASSERT_PIXELS_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "libshadertrap/command.h"
#include "libshadertrap/token.h"

namespace shadertrap {

class CommandAssertPixels : public Command {
 public:
  CommandAssertPixels(std::unique_ptr<Token> start_token, uint8_t expected_r,
                      uint8_t expected_g, uint8_t expected_b,
                      uint8_t expected_a,
                      std::unique_ptr<Token> renderbuffer_identifier,
                      size_t rectangle_x, size_t rectangle_y,
                      size_t rectangle_width, size_t rectangle_height,
                      std::unique_ptr<Token> rectangle_width_token,
                      std::unique_ptr<Token> rectangle_height_token);

  bool Accept(CommandVisitor* visitor) override;

  uint8_t GetExpectedR() const { return expected_r_; }

  uint8_t GetExpectedG() const { return expected_g_; }

  uint8_t GetExpectedB() const { return expected_b_; }

  uint8_t GetExpectedA() const { return expected_a_; }

  const std::string& GetRenderbufferIdentifier() const {
    return renderbuffer_identifier_->GetText();
  }

  const Token& GetRenderbufferIdentifierToken() const {
    return *renderbuffer_identifier_;
  }

  size_t GetRectangleX() const { return rectangle_x_; }

  size_t GetRectangleY() const { return rectangle_y_; }

  size_t GetRectangleWidth() const { return rectangle_width_; }

  size_t GetRectangleHeight() const { return rectangle_height_; }

  const Token& GetRectangleWidthToken() const {
    return *rectangle_width_token_;
  }

  const Token& GetRectangleHeightToken() const {
    return *rectangle_height_token_;
  }

 private:
  uint8_t expected_r_;
  uint8_t expected_g_;
  uint8_t expected_b_;
  uint8_t expected_a_;
  std::unique_ptr<Token> renderbuffer_identifier_;
  size_t rectangle_x_;
  size_t rectangle_y_;
  size_t rectangle_width_;
  size_t rectangle_height_;
  std::unique_ptr<Token> rectangle_width_token_;
  std::unique_ptr<Token> rectangle_height_token_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_ASSERT_PIXELS_H
