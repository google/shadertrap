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

#include "libshadertrap/parser.h"

#include "libshadertraptest/collecting_message_consumer.h"
#include "libshadertraptest/gtest.h"

namespace shadertrap {
namespace {

TEST(ParserTest, NoShaders) {
  std::string program = R"(CREATE_PROGRAM prog SHADERS
    )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_TRUE(message_consumer.GetMessageString(0).find(
                  "Expected the identifier of at least one compiled shader") !=
              std::string::npos);
}

TEST(ParserTest, ShaderStartsOnSameLineAsDeclaration) {
  std::string program = R"(DECLARE_SHADER s FRAGMENT version 320 es
void main() {
}
END
    )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 1:18: Shader text should begin on the line directly following "
      "the 'FRAGMENT' keyword",
      message_consumer.GetMessageString(0));
}

TEST(ParserTest, WarningIfVersionStringStartsOnSameLineAsDeclaration) {
  std::string program = R"(DECLARE_SHADER s FRAGMENT        #version 320 es
void main() {
}
END
    )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "WARNING: 1:34: '#version ...' will be treated as a comment. If it is "
      "supposed to be the first line of shader code, it should start on the "
      "following line",
      message_consumer.GetMessageString(0));
}

}  // namespace
}  // namespace shadertrap
