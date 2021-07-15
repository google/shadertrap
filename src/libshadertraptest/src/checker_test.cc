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

#include "libshadertrap/checker.h"

#include <memory>

#include "libshadertrap/glslang.h"
#include "libshadertrap/parser.h"
#include "libshadertrap/shadertrap_program.h"
#include "libshadertraptest/collecting_message_consumer.h"
#include "libshadertraptest/gtest.h"

namespace shadertrap {
namespace {

void EnsureGlslangIsInitialized() {
  class GlslangInitializer {
   public:
    GlslangInitializer() { ShInitialize(); }
    ~GlslangInitializer() { ShFinalize(); }
    GlslangInitializer(const GlslangInitializer&) = delete;
    GlslangInitializer& operator=(const GlslangInitializer&) = delete;
    GlslangInitializer(GlslangInitializer&&) = delete;
    GlslangInitializer& operator=(GlslangInitializer&&) = delete;
  };
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif
  static GlslangInitializer initializer;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
}

class CheckerTestFixture : public ::testing::Test {
 public:
  void SetUp() override {
    // Initialize glslang.
    EnsureGlslangIsInitialized();
  }
};

TEST_F(CheckerTestFixture, RedeclareShader) {
  std::string program = R"(GLES 3.1
DECLARE_SHADER s KIND VERTEX
#version 320 es
layout(location = 0) in vec2 _GLF_vertexPosition;
void main(void) {
    gl_Position = vec4(_GLF_vertexPosition, 0.0, 1.0);
}
END

DECLARE_SHADER s KIND VERTEX
#version 320 es
layout(location = 0) in vec2 _GLF_vertexPosition;
void main(void) {
    gl_Position = vec4(_GLF_vertexPosition, 0.0, 1.0);
}
END
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 10:16: Identifier 's' already used at 2:16",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, GlslangParseError) {
  // glslang should fail to parse the program.
  std::string program = R"(GLES 3.1
DECLARE_SHADER s KIND VERTEX
notversion
END
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_TRUE(message_consumer.GetMessageString(0).find(
                  "ERROR: 2:1: Validation of shader 's' using glslang failed "
                  "with the following messages:") != std::string::npos);
  ASSERT_TRUE(message_consumer.GetMessageString(0).find(
                  "ERROR: line 3: '' :  syntax error, unexpected IDENTIFIER") !=
              std::string::npos);
}

TEST_F(CheckerTestFixture, GlslangPrecisionError) {
  // glslang will complain that a float is declared with no default precision
  // qualifier.
  std::string program = R"(GLES 3.1
DECLARE_SHADER s KIND FRAGMENT
#version 320 es
float f;
void main() {
}
END
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_TRUE(message_consumer.GetMessageString(0).find(
                  "ERROR: 2:1: Validation of shader 's' using glslang failed "
                  "with the following messages:") != std::string::npos);
  ASSERT_TRUE(
      message_consumer.GetMessageString(0).find(
          "ERROR: line 4: 'float' : type requires declaration of default "
          "precision qualifier") != std::string::npos);
}

TEST_F(CheckerTestFixture, CompileShaderUnknownShader) {
  std::string program = R"(GLES 3.1
COMPILE_SHADER result SHADER nonexistent
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 2:30: Identifier 'nonexistent' does not correspond to a declared "
      "shader",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CompileShaderNameAlreadyUsed) {
  std::string program = R"(GLES 3.1
DECLARE_SHADER s KIND VERTEX
#version 320 es
layout(location = 0) in vec2 _GLF_vertexPosition;
void main(void) {
    gl_Position = vec4(_GLF_vertexPosition, 0.0, 1.0);
}
END

COMPILE_SHADER s SHADER s
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 10:16: Identifier 's' already used at 2:16",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateBufferNameAlreadyUsed) {
  std::string program = R"(GLES 3.1
DECLARE_SHADER vert KIND VERTEX
#version 320 es
void main() { }
END

CREATE_BUFFER vert SIZE_BYTES 8 INIT_VALUES float 1.0 2.0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 7:15: Identifier 'vert' already used at 2:16",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramUnknownShader) {
  std::string program = R"(GLES 3.1
DECLARE_SHADER vert KIND VERTEX
#version 320 es
void main() { }
END

DECLARE_SHADER frag KIND FRAGMENT
#version 320 es
void main() { }
END

COMPILE_SHADER vert_compiled SHADER vert
COMPILE_SHADER frag_compiled SHADER frag
CREATE_SAMPLER mysampler
CREATE_PROGRAM prog SHADERS vert_compiled mysampler frag_compiled
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 15:43: Identifier 'mysampler' does not correspond to a compiled "
      "shader",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramNameAlreadyUsed) {
  std::string program = R"(GLES 3.1
DECLARE_SHADER vert KIND VERTEX
#version 320 es
void main() { }
END

DECLARE_SHADER frag KIND FRAGMENT
#version 320 es
void main() { }
END

COMPILE_SHADER vert_compiled SHADER vert
COMPILE_SHADER frag_compiled SHADER frag
CREATE_PROGRAM frag_compiled SHADERS vert_compiled frag_compiled
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 14:16: Identifier 'frag_compiled' already used at 13:16",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramNoFragmentShader) {
  std::string program = R"(GLES 3.1
DECLARE_SHADER vert KIND VERTEX
#version 320 es
void main() { }
END

COMPILE_SHADER vert_compiled SHADER vert
CREATE_PROGRAM prog SHADERS vert_compiled
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 8:1: No fragment shader provided for 'CREATE_PROGRAM' command",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramNoVertexShader) {
  std::string program = R"(GLES 3.1
DECLARE_SHADER frag KIND FRAGMENT
#version 320 es
void main() { }
END

COMPILE_SHADER frag_compiled SHADER frag
CREATE_PROGRAM prog SHADERS frag_compiled
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 8:1: No vertex shader provided for 'CREATE_PROGRAM' command",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramMultipleFragmentShaders) {
  std::string program = R"(GLES 3.1
DECLARE_SHADER vert KIND VERTEX
#version 320 es
void main() { }
END

DECLARE_SHADER frag KIND FRAGMENT
#version 320 es
void main() { }
END

COMPILE_SHADER vert_compiled SHADER vert
COMPILE_SHADER frag_compiled SHADER frag
CREATE_PROGRAM prog SHADERS vert_compiled frag_compiled frag_compiled
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 14:57: Multiple fragment shaders provided to 'CREATE_PROGRAM'; "
      "already found 'frag_compiled' at 14:43",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramMultipleVertexShaders) {
  std::string program = R"(GLES 3.1
DECLARE_SHADER vert KIND VERTEX
#version 320 es
void main() { }
END

DECLARE_SHADER frag KIND FRAGMENT
#version 320 es
void main() { }
END

COMPILE_SHADER vert_compiled SHADER vert
COMPILE_SHADER frag_compiled SHADER frag
CREATE_PROGRAM prog SHADERS frag_compiled vert_compiled vert_compiled
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 14:57: Multiple vertex shaders provided to 'CREATE_PROGRAM'; "
      "already found 'vert_compiled' at 14:43",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramMultipleComputeShaders) {
  std::string program = R"(GLES 3.1
DECLARE_SHADER comp KIND COMPUTE
#version 320 es
void main() { }
END

COMPILE_SHADER comp_compiled SHADER comp
CREATE_PROGRAM prog SHADERS comp_compiled comp_compiled
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 8:43: Multiple compute shaders provided to 'CREATE_PROGRAM'; "
      "already found 'comp_compiled' at 8:29",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramComputeAndFragmentShaders) {
  std::string program = R"(GLES 3.1
DECLARE_SHADER comp KIND COMPUTE
#version 320 es
void main() { }
END

DECLARE_SHADER frag KIND FRAGMENT
#version 320 es
void main() { }
END

COMPILE_SHADER comp_compiled SHADER comp
COMPILE_SHADER frag_compiled SHADER frag
CREATE_PROGRAM prog SHADERS frag_compiled comp_compiled
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 14:43: A compute shader cannot be used in 'CREATE_PROGRAM' with "
      "another "
      "kind of shader; "
      "found fragment shader 'frag_compiled' at 14:29",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramComputeAndVertexShaders) {
  std::string program = R"(GLES 3.1
DECLARE_SHADER comp KIND COMPUTE
#version 320 es
void main() { }
END

DECLARE_SHADER vert KIND VERTEX
#version 320 es
void main() { }
END

COMPILE_SHADER comp_compiled SHADER comp
COMPILE_SHADER vert_compiled SHADER vert
CREATE_PROGRAM prog SHADERS comp_compiled vert_compiled
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 14:29: A compute shader cannot be used in 'CREATE_PROGRAM' with "
      "another "
      "kind of shader; "
      "found vertex shader 'vert_compiled' at 14:43",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateRenderbufferNameAlreadyUsed) {
  std::string program = R"(GLES 3.1
DECLARE_SHADER vert KIND VERTEX
#version 320 es
void main() { }
END

CREATE_RENDERBUFFER vert WIDTH 24 HEIGHT 24
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 7:21: Identifier 'vert' already used at 2:16",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateSamplerNameAlreadyUsed) {
  std::string program = R"(GLES 3.1
CREATE_EMPTY_TEXTURE_2D name WIDTH 12 HEIGHT 12
CREATE_SAMPLER name
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:16: Identifier 'name' already used at 2:25",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateEmptyTexture2DNameAlreadyUsed) {
  std::string program = R"(GLES 3.1
CREATE_EMPTY_TEXTURE_2D name WIDTH 12 HEIGHT 12
CREATE_EMPTY_TEXTURE_2D name WIDTH 12 HEIGHT 12
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:25: Identifier 'name' already used at 2:25",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualDifferentWidthRenderbuffers) {
  std::string program = R"(GLES 3.1
CREATE_RENDERBUFFER buf1 WIDTH 1 HEIGHT 1
CREATE_RENDERBUFFER buf2 WIDTH 2 HEIGHT 1
ASSERT_EQUAL RENDERBUFFERS buf1 buf2
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 4:33: width 2 of 'buf2' does not match width 1 of 'buf1' at 4:28",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualDifferentHeightRenderbuffers) {
  std::string program = R"(GLES 3.1
CREATE_RENDERBUFFER buf1 WIDTH 1 HEIGHT 1
CREATE_RENDERBUFFER buf2 WIDTH 1 HEIGHT 2
ASSERT_EQUAL RENDERBUFFERS buf1 buf2
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 4:33: height 2 of 'buf2' does not match height 1 of 'buf1' at "
      "4:28",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualDifferentSizedBuffers) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf1 SIZE_BYTES 4 INIT_VALUES uint 0
CREATE_BUFFER buf2 SIZE_BYTES 8 INIT_VALUES uint 0 0
ASSERT_EQUAL BUFFERS buf1 buf2
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 4:27: size (in bytes) 8 of 'buf2' does not match size (in bytes) "
      "4 of 'buf1' at 4:22",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualBufferVsRenderbuffer) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf1 SIZE_BYTES 4 INIT_VALUES uint 0
CREATE_RENDERBUFFER buf2 WIDTH 1 HEIGHT 1
ASSERT_EQUAL BUFFERS buf1 buf2
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 4:27: 'buf2' must be a buffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualBadFistArgument) {
  std::string program = R"(GLES 3.1
CREATE_RENDERBUFFER buf2 WIDTH 24 HEIGHT 24
ASSERT_EQUAL RENDERBUFFERS buf1 buf2
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:28: 'buf1' must be a renderbuffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualBadSecondArgumentRenderbuffer) {
  std::string program = R"(GLES 3.1
CREATE_RENDERBUFFER buf1 WIDTH 24 HEIGHT 24
CREATE_SAMPLER buf2
ASSERT_EQUAL RENDERBUFFERS buf1 buf2
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 4:33: 'buf2' must be a renderbuffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualBadSecondArgumentBuffer) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf1 SIZE_BYTES 4 INIT_VALUES uint 0
CREATE_SAMPLER buf2
ASSERT_EQUAL BUFFERS buf1 buf2
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 4:27: 'buf2' must be a buffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertPixelsNotRenderbuffer) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 4 INIT_VALUES int 0
ASSERT_PIXELS RENDERBUFFER buf RECTANGLE 0 0 2 2 EXPECTED 0 0 0 0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:28: 'buf' is not a renderbuffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertPixelsOutOfBoundsX) {
  std::string program =
      R"(GLES 3.1
CREATE_RENDERBUFFER buf WIDTH 16 HEIGHT 16
ASSERT_PIXELS RENDERBUFFER buf RECTANGLE 8 8 9 8 EXPECTED 0 0 0 0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:46: rectangle extends to x-coordinate 17, which exceeds width "
      "16 of 'buf'",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertPixelsOutOfBoundsY) {
  std::string program =
      R"(GLES 3.1
CREATE_RENDERBUFFER buf WIDTH 16 HEIGHT 16
ASSERT_PIXELS RENDERBUFFER buf RECTANGLE 8 8 8 9 EXPECTED 0 0 0 0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:48: rectangle extends to y-coordinate 17, which exceeds height "
      "16 of 'buf'",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertPixelsZeroWidthRectangle) {
  std::string program =
      R"(GLES 3.1
CREATE_RENDERBUFFER buf WIDTH 16 HEIGHT 16
ASSERT_PIXELS RENDERBUFFER buf RECTANGLE 8 8 0 8 EXPECTED 0 0 0 0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:46: width of rectangle must be positive",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertPixelsZeroHeightRectangle) {
  std::string program =
      R"(GLES 3.1
CREATE_RENDERBUFFER buf WIDTH 16 HEIGHT 16
ASSERT_PIXELS RENDERBUFFER buf RECTANGLE 8 8 8 0 EXPECTED 0 0 0 0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:48: height of rectangle must be positive",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertSimilarEmdHistogramBadFistArgument) {
  std::string program = R"(GLES 3.1
CREATE_RENDERBUFFER buf2 WIDTH 24 HEIGHT 24
ASSERT_SIMILAR_EMD_HISTOGRAM RENDERBUFFERS buf1 buf2 TOLERANCE 1.0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:44: 'buf1' must be a renderbuffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertSimilarEmdHistogramBadSecondArgument) {
  std::string program = R"(GLES 3.1
CREATE_RENDERBUFFER buf1 WIDTH 24 HEIGHT 24
ASSERT_SIMILAR_EMD_HISTOGRAM RENDERBUFFERS buf1 buf2 TOLERANCE 1.0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:49: 'buf2' must be a renderbuffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertSimilarEmdHistogramDifferentWidths) {
  std::string program = R"(GLES 3.1
CREATE_RENDERBUFFER buf1 WIDTH 24 HEIGHT 24
CREATE_RENDERBUFFER buf2 WIDTH 20 HEIGHT 24
ASSERT_SIMILAR_EMD_HISTOGRAM RENDERBUFFERS buf1 buf2 TOLERANCE 1.0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 4:49: width 20 of 'buf2' does not match width 24 of 'buf1' at "
      "4:44",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertSimilarEmdHistogramDifferentHeights) {
  std::string program = R"(GLES 3.1
CREATE_RENDERBUFFER buf1 WIDTH 24 HEIGHT 24
CREATE_RENDERBUFFER buf2 WIDTH 24 HEIGHT 28
ASSERT_SIMILAR_EMD_HISTOGRAM RENDERBUFFERS buf1 buf2 TOLERANCE 1.0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 4:49: height 28 of 'buf2' does not match height 24 of 'buf1' at "
      "4:44",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, BindSamplerBadSampler) {
  std::string program = R"(GLES 3.1
BIND_SAMPLER SAMPLER doesnotexist TEXTURE_UNIT 1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:22: 'doesnotexist' must be a sampler",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, BindShaderStorageBufferBadStorageBuffer) {
  std::string program =
      R"(GLES 3.1
BIND_SHADER_STORAGE_BUFFER BUFFER doesnotexist BINDING 1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:35: 'doesnotexist' must be a buffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, BindTextureBadTexture) {
  std::string program = R"(GLES 3.1
BIND_TEXTURE TEXTURE doesnotexist TEXTURE_UNIT 1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:22: 'doesnotexist' must be a texture",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, BindUniformBufferBadUniformBuffer) {
  std::string program = R"(GLES 3.1
BIND_UNIFORM_BUFFER BUFFER doesnotexist BINDING 1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:28: 'doesnotexist' must be a buffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, DumpRenderbufferBadRenderbuffer) {
  std::string program =
      R"(GLES 3.1
DUMP_RENDERBUFFER RENDERBUFFER doesnotexist FILE "temp.png"
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:32: 'doesnotexist' must be a renderbuffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, SetUniformBadProgram) {
  std::string program =
      R"(GLES 3.1
SET_UNIFORM PROGRAM prog LOCATION 1 TYPE float VALUES 0.1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:21: 'prog' must be a program",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, RunComputeNonexistentProgram) {
  std::string program =
      R"(GLES 3.1
RUN_COMPUTE PROGRAM prog NUM_GROUPS 1 1 1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:21: 'prog' must be a program",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, RunComputeWithGraphicsProgram) {
  std::string program =
      R"(GLES 3.1
DECLARE_SHADER frag KIND FRAGMENT
void main() { }
END
DECLARE_SHADER vert KIND VERTEX
void main() { }
END
COMPILE_SHADER frag_compiled SHADER frag
COMPILE_SHADER vert_compiled SHADER vert
CREATE_PROGRAM prog SHADERS frag_compiled vert_compiled
RUN_COMPUTE PROGRAM prog NUM_GROUPS 1 1 1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 11:21: 'prog' must be a compute program, not a graphics program",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, RunGraphicsNonexistentProgram) {
  std::string program =
      R"(GLES 3.1
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
    gl_Position = vec4(pos, 0.0, 1.0);
}
END

COMPILE_SHADER frag_compiled SHADER frag

COMPILE_SHADER vert_compiled SHADER vert

CREATE_BUFFER vertex_buffer SIZE_BYTES 24 INIT_VALUES float
                           0.0 -1.0
                           -1.0 1.0
                            1.0 1.0

CREATE_BUFFER index_buffer SIZE_BYTES 24 INIT_VALUES uint
                           0 1 2 3 4 5

CREATE_RENDERBUFFER renderbuffer WIDTH 256 HEIGHT 256

RUN_GRAPHICS
  PROGRAM nonexistent
  VERTEX_DATA
    [ 0 -> BUFFER vertex_buffer OFFSET_BYTES 0 STRIDE_BYTES 8 DIMENSION 2 ]
  INDEX_DATA index_buffer
  VERTEX_COUNT 3
  TOPOLOGY TRIANGLES
  FRAMEBUFFER_ATTACHMENTS
    [ 0 -> renderbuffer ]
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 34:11: 'nonexistent' must be a program",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, RunGraphicsNonexistentVertexBuffer) {
  std::string program =
      R"(GLES 3.1
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
    gl_Position = vec4(pos, 0.0, 1.0);
}
END

COMPILE_SHADER frag_compiled SHADER frag

COMPILE_SHADER vert_compiled SHADER vert

CREATE_PROGRAM program SHADERS vert_compiled frag_compiled

CREATE_BUFFER index_buffer SIZE_BYTES 24 INIT_VALUES uint
                           0 1 2 3 4 5

CREATE_RENDERBUFFER renderbuffer WIDTH 256 HEIGHT 256

RUN_GRAPHICS
  PROGRAM program
  VERTEX_DATA
    [ 0 -> BUFFER nonexistent OFFSET_BYTES 0 STRIDE_BYTES 8 DIMENSION 2 ]
  INDEX_DATA index_buffer
  VERTEX_COUNT 3
  TOPOLOGY TRIANGLES
  FRAMEBUFFER_ATTACHMENTS
    [ 0 -> renderbuffer ]
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 33:19: vertex buffer 'nonexistent' must be a buffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, RunGraphicsNonexistentIndexBuffer) {
  std::string program =
      R"(GLES 3.1
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
    gl_Position = vec4(pos, 0.0, 1.0);
}
END

COMPILE_SHADER frag_compiled SHADER frag

COMPILE_SHADER vert_compiled SHADER vert

CREATE_PROGRAM program SHADERS vert_compiled frag_compiled

CREATE_BUFFER vertex_buffer SIZE_BYTES 24 INIT_VALUES float
                           0.0 -1.0
                           -1.0 1.0
                            1.0 1.0

CREATE_RENDERBUFFER renderbuffer WIDTH 256 HEIGHT 256

RUN_GRAPHICS
  PROGRAM program
  VERTEX_DATA
    [ 0 -> BUFFER vertex_buffer OFFSET_BYTES 0 STRIDE_BYTES 8 DIMENSION 2 ]
  INDEX_DATA nonexistent
  VERTEX_COUNT 3
  TOPOLOGY TRIANGLES
  FRAMEBUFFER_ATTACHMENTS
    [ 0 -> renderbuffer ]
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 36:14: index buffer 'nonexistent' must be a buffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, RunGraphicsNonexistentFramebufferAttachment) {
  std::string program =
      R"(GLES 3.1
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
    gl_Position = vec4(pos, 0.0, 1.0);
}
END

COMPILE_SHADER frag_compiled SHADER frag

COMPILE_SHADER vert_compiled SHADER vert

CREATE_PROGRAM program SHADERS vert_compiled frag_compiled

CREATE_BUFFER vertex_buffer SIZE_BYTES 24 INIT_VALUES float
                           0.0 -1.0
                           -1.0 1.0
                            1.0 1.0

CREATE_BUFFER index_buffer SIZE_BYTES 24 INIT_VALUES uint
                           0 1 2 3 4 5

RUN_GRAPHICS
  PROGRAM program
  VERTEX_DATA
    [ 0 -> BUFFER vertex_buffer OFFSET_BYTES 0 STRIDE_BYTES 8 DIMENSION 2 ]
  INDEX_DATA index_buffer
  VERTEX_COUNT 3
  TOPOLOGY TRIANGLES
  FRAMEBUFFER_ATTACHMENTS
    [ 0 -> nonexistent ]
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 41:12: framebuffer attachment 'nonexistent' must be a "
      "renderbuffer or texture",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, RunGraphicsWithComputeProgram) {
  std::string program =
      R"(GLES 3.1
DECLARE_SHADER comp KIND COMPUTE
#version 320 es
void main() { }
END

COMPILE_SHADER comp_compiled SHADER comp

CREATE_PROGRAM program SHADERS comp_compiled

CREATE_BUFFER vertex_buffer SIZE_BYTES 24 INIT_VALUES float
                           0.0 -1.0
                           -1.0 1.0
                            1.0 1.0

CREATE_BUFFER index_buffer SIZE_BYTES 24 INIT_VALUES uint
                           0 1 2 3 4 5

CREATE_RENDERBUFFER renderbuffer WIDTH 256 HEIGHT 256

RUN_GRAPHICS
  PROGRAM program
  VERTEX_DATA
    [ 0 -> BUFFER vertex_buffer OFFSET_BYTES 0 STRIDE_BYTES 8 DIMENSION 2 ]
  INDEX_DATA index_buffer
  VERTEX_COUNT 3
  TOPOLOGY TRIANGLES
  FRAMEBUFFER_ATTACHMENTS
    [ 0 -> renderbuffer ]
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 22:11: 'program' must be a graphics program, not a compute "
      "program",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, SetSamplerParameterNonexistentSampler) {
  std::string program =
      R"(GLES 3.1
SET_SAMPLER_PARAMETER PARAMETER TEXTURE_MAG_FILTER VALUE LINEAR SAMPLER nonexistent
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:73: 'nonexistent' must be a sampler",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, SetTextureParameterNonexistentSampler) {
  std::string program =
      R"(GLES 3.1
SET_TEXTURE_PARAMETER VALUE NEAREST PARAMETER TEXTURE_MIN_FILTER TEXTURE nonexistent
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:74: 'nonexistent' must be a texture",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, NoComputeShaderBeforeGl43) {
  std::string program =
      R"(GL 4.2
DECLARE_SHADER comp KIND COMPUTE
#version 320 es
void main() { }
END
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 2:1: Compute shaders are not supported before OpenGL 4.3 or "
      "OpenGL ES 3.1",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, NoComputeShaderBeforeGles31) {
  std::string program =
      R"(GLES 3.0
DECLARE_SHADER comp KIND COMPUTE
#version 320 es
void main() { }
END
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 2:1: Compute shaders are not supported before OpenGL 4.3 or "
      "OpenGL ES 3.1",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, OnlyUseColorAttachment0WithGles2) {
  std::string program =
      R"(GLES 2.0

DECLARE_SHADER frag KIND FRAGMENT
#version 100
precision highp float;
void main() {
 gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
END

DECLARE_SHADER vert KIND VERTEX
#version 100
varying vec2 a_pos;
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
    [ 1 -> renderbuffer ]
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 43:12: Only 0 may be used as a framebuffer attachment key when "
      "working with OpenGL ES 2.0",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, DumpBufferBinaryUnknownBuffer) {
  std::string program =
      R"(GLES 3.1
DUMP_BUFFER_BINARY BUFFER doesnotexist FILE "temp.bin"
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:27: 'doesnotexist' must be a buffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, DumpBufferBinaryNotBuffer) {
  std::string program =
      R"(GLES 3.1
CREATE_RENDERBUFFER renderbuf WIDTH 24 HEIGHT 24
DUMP_BUFFER_BINARY BUFFER renderbuf FILE "temp.bin"
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:27: 'renderbuf' must be a buffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, DumpBufferTextUnknownBuffer) {
  std::string program =
      R"(GLES 3.1
DUMP_BUFFER_TEXT BUFFER doesnotexist FILE "temp.txt" FORMAT float 4
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:25: 'doesnotexist' must be a buffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, DumpBufferTextNotBuffer) {
  std::string program =
      R"(GLES 3.1
CREATE_RENDERBUFFER renderbuf WIDTH 24 HEIGHT 24
DUMP_BUFFER_TEXT BUFFER renderbuf FILE "temp.txt" FORMAT float 4
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:25: 'renderbuf' must be a buffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, DumpBufferTextGood) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 48 INIT_VALUES uint 0 0 0 0 0 0 0 0 0 0 0 0
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT float 12
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT uint 12
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT int 12
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT byte 48
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT int 4 float 2 float 2 uint 1 uint 3
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT byte 16 float 2 SKIP_BYTES 8 uint 1 uint 3
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT "hello" byte 16 "\nworld" "hello" float 2 "again\n" SKIP_BYTES 8 uint 1 uint 3
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_TRUE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(0, message_consumer.GetNumMessages());
}

TEST_F(CheckerTestFixture, DumpBufferTextNotEnoughElements) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 48 INIT_VALUES uint 0 0 0 0 0 0 0 0 0 0 0 0
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT float 11
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:52: The number of bytes specified in the formatting of 'buf' "
      "is 44, but 'buf' was declared with size 48 bytes at 2:1",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, DumpBufferTextTooManyElements) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 48 INIT_VALUES uint 0 0 0 0 0 0 0 0 0 0 0 0
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT float 13
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:52: The number of bytes specified in the formatting of 'buf' "
      "is 52, but 'buf' was declared with size 48 bytes at 2:1",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, DumpBufferTextByteNotMultipleOfFour) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 48 INIT_VALUES uint 0 0 0 0 0 0 0 0 0 0 0 0
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT byte 1 float 11 byte 3
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(2, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:52: The count for a 'byte' formatting entry must be a multiple "
      "of 4; found 1",
      message_consumer.GetMessageString(0));
  ASSERT_EQ(
      "ERROR: 3:68: The count for a 'byte' formatting entry must be a multiple "
      "of 4; found 3",
      message_consumer.GetMessageString(1));
}

TEST_F(CheckerTestFixture, DumpBufferTextSkipNotMultipleOfFour) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 48 INIT_VALUES uint 0 0 0 0 0 0 0 0 0 0 0 0
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT SKIP_BYTES 1 float 11 SKIP_BYTES 3
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(2, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:52: The count for a 'SKIP_BYTES' formatting entry must be a "
      "multiple of 4; found 1",
      message_consumer.GetMessageString(0));
  ASSERT_EQ(
      "ERROR: 3:74: The count for a 'SKIP_BYTES' formatting entry must be a "
      "multiple of 4; found 3",
      message_consumer.GetMessageString(1));
}

TEST_F(CheckerTestFixture, DumpBufferSkipBytesCountCannotBeZero) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 48 INIT_VALUES uint 0 0 0 0 0 0 0 0 0 0 0 0
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT SKIP_BYTES 0 float 12
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:52: The count for a formatting entry must be positive",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, DumpBufferByteCountCannotBeZero) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 48 INIT_VALUES uint 0 0 0 0 0 0 0 0 0 0 0 0
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT byte 0 float 12
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:52: The count for a formatting entry must be positive",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, DumpBufferFloatCountCannotBeZero) {
  std::string program =
      R"(GLES 3.1
CREATE_BUFFER buf SIZE_BYTES 48 INIT_VALUES uint 0 0 0 0 0 0 0 0 0 0 0 0
DUMP_BUFFER_TEXT BUFFER buf FILE "temp.txt" FORMAT float 0 float 12
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 3:52: The count for a formatting entry must be positive",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualFailCheckerTest1) {
  std::string program =
      R"(GL 4.5
CREATE_BUFFER buf1 SIZE_BYTES 16 INIT_VALUES float 1.0 2.0 3.0 4.0
CREATE_BUFFER buf2 SIZE_BYTES 16 INIT_VALUES float 1.0 2.0 3.0 4.0
ASSERT_EQUAL BUFFERS buf1 buf2 FORMAT float 3
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 4:39: The number of bytes specified in the formatting of "
      "'buf1(buf2)' is 12, but 'buf1(buf2)' was declared with size 16 bytes at "
      "2:1",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualFailCheckerTest2) {
  std::string program =
      R"(GL 4.5
CREATE_BUFFER buf1 SIZE_BYTES 16 INIT_VALUES float 1.0 2.0 3.0 4.0
CREATE_BUFFER buf2 SIZE_BYTES 16 INIT_VALUES float 1.0 2.0 3.0 4.0
ASSERT_EQUAL BUFFERS buf1 buf2 FORMAT float 0 float 4
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 4:39: The count for a formatting entry must be positive",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualFailCheckerTest3) {
  std::string program =
      R"(GL 4.5
CREATE_BUFFER buf1 SIZE_BYTES 16 INIT_VALUES float 1.0 2.0 3.0 4.0
CREATE_BUFFER buf2 SIZE_BYTES 16 INIT_VALUES float 1.0 2.0 3.0 4.0
ASSERT_EQUAL BUFFERS buf1 buf2 FORMAT SKIP_BYTES 0 float 4
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 4:39: The count for a formatting entry must be positive",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualFailCheckerTest4) {
  std::string program =
      R"(GL 4.5
CREATE_BUFFER buf1 SIZE_BYTES 16 INIT_VALUES float 1.0 2.0 3.0 4.0
CREATE_BUFFER buf2 SIZE_BYTES 16 INIT_VALUES float 1.0 2.0 3.0 4.0
ASSERT_EQUAL BUFFERS buf1 buf2 FORMAT SKIP_BYTES 3 float 4
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  auto parsed_program = parser.GetParsedProgram();
  Checker checker(&message_consumer, parsed_program->GetApiVersion());
  ASSERT_FALSE(checker.VisitCommands(parsed_program.get()));
  ASSERT_EQ(2, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 4:39: The count for a 'SKIP_BYTES' formatting entry must be a "
      "multiple of 4; found 3",
      message_consumer.GetMessageString(0));
}

}  // namespace
}  // namespace shadertrap
