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

#include "libshadertrap/command_create_buffer.h"
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

TEST(ParserTest, SetUniformNameAndValue) {
  std::string program =
      R"(DECLARE_SHADER shader COMPUTE
layout(location = 1) uniform float f;
void main() {
  f;
}
END
COMPILE_SHADER shader_compiled SHADER shader
CREATE_PROGRAM compute_program SHADERS shader_compiled
SET_UNIFORM PROGRAM compute_program LOCATION 1 NAME f TYPE float VALUES 1.0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 9:37: Parameters 'LOCATION' and 'NAME' are mutually exclusive; "
      "both are present at 9:37 and 9:48",
      message_consumer.GetMessageString(0));
}

TEST(ParserTest, CreateBufferVariousTypes) {
  std::string program =
      R"(CREATE_BUFFER buf SIZE_BYTES 52 INIT_VALUES
   INT 1 2 3
   FLOAT 1.0 2.0 3.0
   UINT 10 11 12
   BYTE 1 2 3 4
   INT 4 5 6
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto data = reinterpret_cast<CommandCreateBuffer*>(
                  parser.GetParsedProgram()->GetCommand(0))
                  ->GetData();

  int32_t int_temp;
  memcpy(&int_temp, data.data(), sizeof(int32_t));
  ASSERT_EQ(1, int_temp);
  memcpy(&int_temp, data.data() + sizeof(int32_t), sizeof(int32_t));
  ASSERT_EQ(2, int_temp);
  memcpy(&int_temp, data.data() + 2 * sizeof(int32_t), sizeof(int32_t));
  ASSERT_EQ(3, int_temp);

  float float_temp;
  memcpy(&float_temp, data.data() + 3 * sizeof(int32_t), sizeof(float));
  ASSERT_EQ(1.0, float_temp);
  memcpy(&float_temp, data.data() + 3 * sizeof(int32_t) + sizeof(float),
         sizeof(float));
  ASSERT_EQ(2.0, float_temp);
  memcpy(&float_temp, data.data() + 3 * sizeof(int32_t) + 2 * sizeof(float),
         sizeof(float));
  ASSERT_EQ(3.0, float_temp);

  uint32_t uint_temp;
  memcpy(&uint_temp, data.data() + 3 * sizeof(int32_t) + 3 * sizeof(float),
         sizeof(uint32_t));
  ASSERT_EQ(10U, uint_temp);
  memcpy(
      &uint_temp,
      data.data() + 3 * sizeof(int32_t) + 3 * sizeof(float) + sizeof(uint32_t),
      sizeof(uint32_t));
  ASSERT_EQ(11U, uint_temp);
  memcpy(&uint_temp,
         data.data() + 3 * sizeof(int32_t) + 3 * sizeof(float) +
             2 * sizeof(uint32_t),
         sizeof(uint32_t));
  ASSERT_EQ(12U, uint_temp);

  ASSERT_EQ(
      1, data[3 * sizeof(int32_t) + 3 * sizeof(float) + 3 * sizeof(uint32_t)]);
  ASSERT_EQ(
      2,
      data[3 * sizeof(int32_t) + 3 * sizeof(float) + 3 * sizeof(uint32_t) + 1]);
  ASSERT_EQ(
      3,
      data[3 * sizeof(int32_t) + 3 * sizeof(float) + 3 * sizeof(uint32_t) + 2]);
  ASSERT_EQ(
      4,
      data[3 * sizeof(int32_t) + 3 * sizeof(float) + 3 * sizeof(uint32_t) + 3]);

  memcpy(&int_temp,
         data.data() + 3 * sizeof(int32_t) + 3 * sizeof(float) +
             3 * sizeof(uint32_t) + 4,
         sizeof(int32_t));
  ASSERT_EQ(4, int_temp);
  memcpy(&int_temp,
         data.data() + 4 * sizeof(int32_t) + 3 * sizeof(float) +
             3 * sizeof(uint32_t) + 4,
         sizeof(int32_t));
  ASSERT_EQ(5, int_temp);
  memcpy(&int_temp,
         data.data() + 5 * sizeof(int32_t) + 3 * sizeof(float) +
             3 * sizeof(uint32_t) + 4,
         sizeof(int32_t));
  ASSERT_EQ(6, int_temp);
}

TEST(ParserTest, CreateBufferBadByteMultiple) {
  std::string program =
      R"(CREATE_BUFFER buf SIZE_BYTES 3 INIT_VALUES
   INT 3 6
   FLOAT 3.0 BYTE 1 2 3 4 5 6
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:14: The number of byte literals supplied in a buffer "
      "initializer must be a multiple of 4; found a sequence of 6 literals",
      message_consumer.GetMessageString(0));
}

TEST(ParserTest, CreateBufferWrongSize) {
  std::string program =
      R"(CREATE_BUFFER buf SIZE_BYTES 51 INIT_VALUES
   INT 1 2 3
   FLOAT 1.0 2.0 3.0
   UINT 10 11 12
   BYTE 1 2 3 4
   INT 4 5 6
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 1:30: Declared size in bytes 51 does not match the combined size "
      "of the provided initial values, which is 52",
      message_consumer.GetMessageString(0));
}

}  // namespace
}  // namespace shadertrap
