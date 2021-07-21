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

#include <cstring>

#include "libshadertrap/command_create_buffer.h"
#include "libshadertraptest/collecting_message_consumer.h"
#include "libshadertraptest/gtest.h"

namespace shadertrap {
namespace {

TEST(ParserTest, NoShaders) {
  std::string program = R"(GLES 3.1
CREATE_PROGRAM prog SHADERS
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
  std::string program = R"(GLES 3.1
DECLARE_SHADER s KIND FRAGMENT version 320 es
void main() {
}
END
    )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 2:23: Shader text should begin on the line directly following "
      "the 'FRAGMENT' keyword",
      message_consumer.GetMessageString(0));
}

TEST(ParserTest, WarningIfVersionStringStartsOnSameLineAsDeclaration) {
  std::string program = R"(GLES 3.1
DECLARE_SHADER s KIND FRAGMENT        #version 320 es
void main() {
}
END
    )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "WARNING: 2:39: '#version ...' will be treated as a comment. If it is "
      "supposed to be the first line of shader code, it should start on the "
      "following line",
      message_consumer.GetMessageString(0));
}

TEST(ParserTest, SetUniformNameAndValue) {
  std::string program =
      R"(GLES 3.1
DECLARE_SHADER shader KIND COMPUTE
layout(location = 1) uniform float f;
void main() {
  f;
}
END
COMPILE_SHADER shader_compiled SHADER shader
CREATE_PROGRAM compute_program SHADERS shader_compiled
SET_UNIFORM PROGRAM compute_program LOCATION 1 NAME "f" TYPE float VALUES 1.0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 10:37: Parameters 'LOCATION' and 'NAME' are mutually exclusive; "
      "both are present at 10:37 and 10:48",
      message_consumer.GetMessageString(0));
}

TEST(ParserTest, CreateBufferVariousTypes) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 52 INIT_VALUES
   int 1 2 3
   float 1.0 2.0 3.0
   uint 10 11 12
   byte 1 2 3 4
   int 4 5 6
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto data = dynamic_cast<CommandCreateBuffer*>(
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
         data.data() + 5 * sizeof(int32_t) + 3 * sizeof(float) +  // NOLINT
             3 * sizeof(uint32_t) + 4,
         sizeof(int32_t));
  ASSERT_EQ(6, int_temp);
}

TEST(ParserTest, CreateBufferBadByteMultiple) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 3 INIT_VALUES
   int 3 6
   float 3.0 byte 1 2 3 4 5 6
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 4:14: The number of byte literals supplied in a buffer "
      "initializer must be a multiple of 4; found a sequence of 6 literals",
      message_consumer.GetMessageString(0));
}

TEST(ParserTest, CreateBufferWrongSize) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 51 INIT_VALUES
   int 1 2 3
   float 1.0 2.0 3.0
   uint 10 11 12
   byte 1 2 3 4
   int 4 5 6
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 2:30: Declared size in bytes 51 does not match the combined size "
      "of the provided initial values, which is 52",
      message_consumer.GetMessageString(0));
}

TEST(ParserTest, NoDuplicateColorAttachmentKeys) {
  std::string program =
      R"(GLES 3.2

DECLARE_SHADER frag KIND FRAGMENT
#version 320 es
precision highp float;
layout(location = 0) out vec4 color;
void main() {
 color = vec4(1.0, 0.0, 0.0, 1.0);
}
END

DECLARE_SHADER vert KIND VERTEX
#version 320 es
layout(location = 0) in vec2 pos;
void main(void) {
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
END

COMPILE_SHADER frag_compiled SHADER frag

COMPILE_SHADER vert_compiled SHADER vert

CREATE_PROGRAM program SHADERS vert_compiled frag_compiled

CREATE_BUFFER vertex_buffer SIZE_BYTES 24 INIT_VALUES float
                           0.0 -1.0
                           -1.0 1.0
                            1.0 1.0

CREATE_BUFFER index_buffer SIZE_BYTES 12 INIT_VALUES uint
                           0 1 2

CREATE_RENDERBUFFER renderbuffer WIDTH 256 HEIGHT 256
CREATE_RENDERBUFFER renderbuffer2 WIDTH 256 HEIGHT 256

RUN_GRAPHICS
  PROGRAM program
  VERTEX_DATA
    [ 0 -> BUFFER vertex_buffer OFFSET_BYTES 0 STRIDE_BYTES 8 DIMENSION 2 ]
  INDEX_DATA index_buffer
  VERTEX_COUNT 3
  TOPOLOGY TRIANGLES
  FRAMEBUFFER_ATTACHMENTS
    [ 0 -> renderbuffer, 0 -> renderbuffer2 ]
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 45:26: Duplicate key: 0 is already used as a key at 45:7",
            message_consumer.GetMessageString(0));
}

TEST(ParserTest, NoDuplicateColorAttachmentValues) {
  std::string program =
      R"(GLES 3.2

DECLARE_SHADER frag KIND FRAGMENT
#version 320 es
precision highp float;
layout(location = 0) out vec4 color;
layout(location = 1) out vec4 color;
void main() {
 color = vec4(1.0, 0.0, 0.0, 1.0);
 color2 = vec4(1.0, 0.0, 0.0, 1.0);
}
END

DECLARE_SHADER vert KIND VERTEX
#version 320 es
layout(location = 0) in vec2 pos;
void main(void) {
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
END

COMPILE_SHADER frag_compiled SHADER frag

COMPILE_SHADER vert_compiled SHADER vert

CREATE_PROGRAM program SHADERS vert_compiled frag_compiled

CREATE_BUFFER vertex_buffer SIZE_BYTES 24 INIT_VALUES float
                           0.0 -1.0
                           -1.0 1.0
                            1.0 1.0

CREATE_BUFFER index_buffer SIZE_BYTES 12 INIT_VALUES uint
                           0 1 2

CREATE_RENDERBUFFER renderbuffer WIDTH 256 HEIGHT 256

RUN_GRAPHICS
  PROGRAM program
  VERTEX_DATA
    [ 0 -> BUFFER vertex_buffer OFFSET_BYTES 0 STRIDE_BYTES 8 DIMENSION 2 ]
  INDEX_DATA index_buffer
  VERTEX_COUNT 3
  TOPOLOGY TRIANGLES
  FRAMEBUFFER_ATTACHMENTS
    [ 0 -> renderbuffer, 1 -> renderbuffer ]
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 46:31: Duplicate attachment: 'renderbuffer' is already attached "
      "at 46:12",
      message_consumer.GetMessageString(0));
}

TEST(ParserTest, DumpBufferTextStringAfterType) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 48 INIT_VALUES uint 0 0 0 0 0 0 0 0 0 0 0 0
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT float "hello"
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:58: Expected integer count, got 'hello'",
            message_consumer.GetMessageString(0));
}

TEST(ParserTest, DumpBufferTextTypeAfterType) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 48 INIT_VALUES uint 0 0 0 0 0 0 0 0 0 0 0 0
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT float float
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:58: Expected integer count, got 'float'",
            message_consumer.GetMessageString(0));
}

TEST(ParserTest, DumpBufferTextNumberWithoutType) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 48 INIT_VALUES uint 0 0 0 0 0 0 0 0 0 0 0 0
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT 3
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:52: Unknown command: '3'",
            message_consumer.GetMessageString(0));
}

TEST(ParserTest, DumpBufferTextNumberAfterNumber) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 48 INIT_VALUES uint 0 0 0 0 0 0 0 0 0 0 0 0
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT float 4 4
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:60: Unknown command: '4'",
            message_consumer.GetMessageString(0));
}

TEST(ParserTest, AssertEqualMissingIdentifier) {
  std::string program =
      R"(GL 4.5
CREATE_BUFFER buf1 SIZE_BYTES 16 INIT_VALUES float 1.0 2.0 3.0 4.0
CREATE_BUFFER buf2 SIZE_BYTES 16 INIT_VALUES float 1.0 2.0 3.0 4.0
ASSERT_EQUAL BUFFERS buf1 buf2 FORMAT
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 4:1: Missing identifier after FORMAT",
            message_consumer.GetMessageString(0));
}

TEST(ParserTest, AssertEqualMissingCount) {
  std::string program =
      R"(GL 4.5
CREATE_BUFFER buf1 SIZE_BYTES 16 INIT_VALUES float 1.0 2.0 3.0 4.0
CREATE_BUFFER buf2 SIZE_BYTES 16 INIT_VALUES float 1.0 2.0 3.0 4.0
ASSERT_EQUAL BUFFERS buf1 buf2 FORMAT float
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 5:1: Expected integer count, got ''",
            message_consumer.GetMessageString(0));
}

TEST(ParserTest, AssertEqualNegativeCount) {
  std::string program =
      R"(GL 4.5
CREATE_BUFFER buf1 SIZE_BYTES 16 INIT_VALUES float 1.0 2.0 3.0 4.0
CREATE_BUFFER buf2 SIZE_BYTES 16 INIT_VALUES float 1.0 2.0 3.0 4.0
ASSERT_EQUAL BUFFERS buf1 buf2 FORMAT float -1
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_FALSE(parser.Parse());
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 4:45: Expected non-negative integer count, got '-1'",
            message_consumer.GetMessageString(0));
}

}  // namespace
}  // namespace shadertrap
