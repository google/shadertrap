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
  std::string program = R"(DECLARE_SHADER s VERTEX
#version 320 es
layout(location = 0) in vec2 _GLF_vertexPosition;
void main(void) {
    gl_Position = vec4(_GLF_vertexPosition, 0.0, 1.0);
}
END

DECLARE_SHADER s VERTEX
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
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 9:16: Identifier 's' already used at 1:16",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, GlslangParseError) {
  // glslang should fail to parse the program.
  std::string program = R"(DECLARE_SHADER s VERTEX
notversion
END
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_TRUE(message_consumer.GetMessageString(0).find(
                  "ERROR: 1:1: Validation of shader 's' using glslang failed "
                  "with the following messages:") != std::string::npos);
  ASSERT_TRUE(message_consumer.GetMessageString(0).find(
                  "ERROR: line 2: '' :  syntax error, unexpected IDENTIFIER") !=
              std::string::npos);
}

TEST_F(CheckerTestFixture, GlslangPrecisionError) {
  // glslang will complain that a float is declared with no default precision
  // qualifier.
  std::string program = R"(DECLARE_SHADER s FRAGMENT
#version 320 es
float f;
void main() {
}
END
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_TRUE(message_consumer.GetMessageString(0).find(
                  "ERROR: 1:1: Validation of shader 's' using glslang failed "
                  "with the following messages:") != std::string::npos);
  ASSERT_TRUE(
      message_consumer.GetMessageString(0).find(
          "ERROR: line 3: 'float' : type requires declaration of default "
          "precision qualifier") != std::string::npos);
}

TEST_F(CheckerTestFixture, CompileShaderUnknownShader) {
  std::string program = R"(COMPILE_SHADER result SHADER nonexistent
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 1:30: Identifier 'nonexistent' does not correspond to a declared "
      "shader",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CompileShaderNameAlreadyUsed) {
  std::string program = R"(DECLARE_SHADER s VERTEX
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
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 9:16: Identifier 's' already used at 1:16",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateBufferNameAlreadyUsed) {
  std::string program = R"(DECLARE_SHADER vert VERTEX
#version 320 es
void main() { }
END

CREATE_BUFFER vert SIZE_BYTES 8 INIT_TYPE float INIT_VALUES 1.0 2.0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 6:15: Identifier 'vert' already used at 1:16",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramUnknownShader) {
  std::string program = R"(DECLARE_SHADER vert VERTEX
#version 320 es
void main() { }
END

DECLARE_SHADER frag FRAGMENT
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
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 14:43: Identifier 'mysampler' does not correspond to a compiled "
      "shader",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramNameAlreadyUsed) {
  std::string program = R"(DECLARE_SHADER vert VERTEX
#version 320 es
void main() { }
END

DECLARE_SHADER frag FRAGMENT
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
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 13:16: Identifier 'frag_compiled' already used at 12:16",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramNoFragmentShader) {
  std::string program = R"(DECLARE_SHADER vert VERTEX
#version 320 es
void main() { }
END

COMPILE_SHADER vert_compiled SHADER vert
CREATE_PROGRAM prog SHADERS vert_compiled
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 7:1: No fragment shader provided for 'CREATE_PROGRAM' command",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramNoVertexShader) {
  std::string program = R"(DECLARE_SHADER frag FRAGMENT
#version 320 es
void main() { }
END

COMPILE_SHADER frag_compiled SHADER frag
CREATE_PROGRAM prog SHADERS frag_compiled
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 7:1: No vertex shader provided for 'CREATE_PROGRAM' command",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramMultipleFragmentShaders) {
  std::string program = R"(DECLARE_SHADER vert VERTEX
#version 320 es
void main() { }
END

DECLARE_SHADER frag FRAGMENT
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
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 13:57: Multiple fragment shaders provided to 'CREATE_PROGRAM'; "
      "already found 'frag_compiled' at 13:43",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramMultipleVertexShaders) {
  std::string program = R"(DECLARE_SHADER vert VERTEX
#version 320 es
void main() { }
END

DECLARE_SHADER frag FRAGMENT
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
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 13:57: Multiple vertex shaders provided to 'CREATE_PROGRAM'; "
      "already found 'vert_compiled' at 13:43",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramMultipleComputeShaders) {
  std::string program = R"(DECLARE_SHADER comp COMPUTE
#version 320 es
void main() { }
END

COMPILE_SHADER comp_compiled SHADER comp
CREATE_PROGRAM prog SHADERS comp_compiled comp_compiled
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 7:43: Multiple compute shaders provided to 'CREATE_PROGRAM'; "
      "already found 'comp_compiled' at 7:29",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramComputeAndFragmentShaders) {
  std::string program = R"(DECLARE_SHADER comp COMPUTE
#version 320 es
void main() { }
END

DECLARE_SHADER frag FRAGMENT
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
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 13:43: A compute shader cannot be used in 'CREATE_PROGRAM' with "
      "another "
      "kind of shader; "
      "found fragment shader 'frag_compiled' at 13:29",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateProgramComputeAndVertexShaders) {
  std::string program = R"(DECLARE_SHADER comp COMPUTE
#version 320 es
void main() { }
END

DECLARE_SHADER vert VERTEX
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
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 13:29: A compute shader cannot be used in 'CREATE_PROGRAM' with "
      "another "
      "kind of shader; "
      "found vertex shader 'vert_compiled' at 13:43",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateRenderbufferNameAlreadyUsed) {
  std::string program = R"(DECLARE_SHADER vert VERTEX
#version 320 es
void main() { }
END

CREATE_RENDERBUFFER vert WIDTH 24 HEIGHT 24
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 6:21: Identifier 'vert' already used at 1:16",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateSamplerNameAlreadyUsed) {
  std::string program = R"(CREATE_EMPTY_TEXTURE_2D name WIDTH 12 HEIGHT 12
CREATE_SAMPLER name
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:16: Identifier 'name' already used at 1:25",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, CreateEmptyTexture2DNameAlreadyUsed) {
  std::string program = R"(CREATE_EMPTY_TEXTURE_2D name WIDTH 12 HEIGHT 12
CREATE_EMPTY_TEXTURE_2D name WIDTH 12 HEIGHT 12
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:25: Identifier 'name' already used at 1:25",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualDifferentWidthRenderbuffers) {
  std::string program = R"(CREATE_RENDERBUFFER buf1 WIDTH 1 HEIGHT 1
CREATE_RENDERBUFFER buf2 WIDTH 2 HEIGHT 1
ASSERT_EQUAL BUFFER1 buf1 BUFFER2 buf2
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:35: width 2 of 'buf2' does not match width 1 of 'buf1' at 3:22",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualDifferentHeightRenderbuffers) {
  std::string program = R"(CREATE_RENDERBUFFER buf1 WIDTH 1 HEIGHT 1
CREATE_RENDERBUFFER buf2 WIDTH 1 HEIGHT 2
ASSERT_EQUAL BUFFER1 buf1 BUFFER2 buf2
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:35: height 2 of 'buf2' does not match height 1 of 'buf1' at "
      "3:22",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualDifferentSizedBuffers) {
  std::string program =
      R"(CREATE_BUFFER buf1 SIZE_BYTES 4 INIT_TYPE uint INIT_VALUES 0
CREATE_BUFFER buf2 SIZE_BYTES 8 INIT_TYPE uint INIT_VALUES 0 0
ASSERT_EQUAL BUFFER2 buf2 BUFFER1 buf1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:22: size (in bytes) 8 of 'buf2' does not match size (in bytes) "
      "4 of 'buf1' at 3:35",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualBufferVsRenderbuffer) {
  std::string program =
      R"(CREATE_BUFFER buf1 SIZE_BYTES 4 INIT_TYPE uint INIT_VALUES 0
CREATE_RENDERBUFFER buf2 WIDTH 1 HEIGHT 1
ASSERT_EQUAL BUFFER2 buf2 BUFFER1 buf1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:22: 'buf1' at 3:35 is a buffer, so 'buf2' must also be a "
      "buffer",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualBadFistArgument) {
  std::string program = R"(CREATE_RENDERBUFFER buf2 WIDTH 24 HEIGHT 24
ASSERT_EQUAL BUFFER1 buf1 BUFFER2 buf2
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:22: 'buf1' must be a buffer or renderbuffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualBadSecondArgumentRenderbuffer) {
  std::string program = R"(CREATE_RENDERBUFFER buf1 WIDTH 24 HEIGHT 24
CREATE_SAMPLER buf2
ASSERT_EQUAL BUFFER1 buf1 BUFFER2 buf2
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:35: 'buf1' at 3:22 is a renderbuffer, so 'buf2' must also be a "
      "renderbuffer",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertEqualBadSecondArgumentBuffer) {
  std::string program =
      R"(CREATE_BUFFER buf1 SIZE_BYTES 4 INIT_TYPE int INIT_VALUES 0
CREATE_SAMPLER buf2
ASSERT_EQUAL BUFFER2 buf2 BUFFER1 buf1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:22: 'buf1' at 3:35 is a buffer, so 'buf2' must also be a "
      "buffer",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertPixelsNotRenderbuffer) {
  std::string program =
      R"(CREATE_BUFFER buf SIZE_BYTES 4 INIT_TYPE int INIT_VALUES 0
ASSERT_PIXELS RENDERBUFFER buf RECTANGLE 0 0 2 2 EXPECTED 0 0 0 0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:28: 'buf' is not a renderbuffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertPixelsOutOfBoundsX) {
  std::string program =
      R"(CREATE_RENDERBUFFER buf WIDTH 16 HEIGHT 16
ASSERT_PIXELS RENDERBUFFER buf RECTANGLE 8 8 9 8 EXPECTED 0 0 0 0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 2:46: rectangle extends to x-coordinate 17, which exceeds width "
      "16 of 'buf'",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertPixelsOutOfBoundsY) {
  std::string program =
      R"(CREATE_RENDERBUFFER buf WIDTH 16 HEIGHT 16
ASSERT_PIXELS RENDERBUFFER buf RECTANGLE 8 8 8 9 EXPECTED 0 0 0 0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 2:48: rectangle extends to y-coordinate 17, which exceeds height "
      "16 of 'buf'",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertPixelsZeroWidthRectangle) {
  std::string program =
      R"(CREATE_RENDERBUFFER buf WIDTH 16 HEIGHT 16
ASSERT_PIXELS RENDERBUFFER buf RECTANGLE 8 8 0 8 EXPECTED 0 0 0 0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:46: width of rectangle must be positive",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertPixelsZeroHeightRectangle) {
  std::string program =
      R"(CREATE_RENDERBUFFER buf WIDTH 16 HEIGHT 16
ASSERT_PIXELS RENDERBUFFER buf RECTANGLE 8 8 8 0 EXPECTED 0 0 0 0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:48: height of rectangle must be positive",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertSimilarEmdHistogramBadFistArgument) {
  std::string program = R"(CREATE_RENDERBUFFER buf2 WIDTH 24 HEIGHT 24
ASSERT_SIMILAR_EMD_HISTOGRAM RENDERBUFFERS buf1 buf2 TOLERANCE 1.0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:44: 'buf1' must be a renderbuffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertSimilarEmdHistogramBadSecondArgument) {
  std::string program = R"(CREATE_RENDERBUFFER buf1 WIDTH 24 HEIGHT 24
ASSERT_SIMILAR_EMD_HISTOGRAM RENDERBUFFERS buf1 buf2 TOLERANCE 1.0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 2:49: 'buf2' must be a renderbuffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertSimilarEmdHistogramDifferentWidths) {
  std::string program = R"(CREATE_RENDERBUFFER buf1 WIDTH 24 HEIGHT 24
CREATE_RENDERBUFFER buf2 WIDTH 20 HEIGHT 24
ASSERT_SIMILAR_EMD_HISTOGRAM RENDERBUFFERS buf1 buf2 TOLERANCE 1.0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:49: width 20 of 'buf2' does not match width 24 of 'buf1' at "
      "3:44",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, AssertSimilarEmdHistogramDifferentHeights) {
  std::string program = R"(CREATE_RENDERBUFFER buf1 WIDTH 24 HEIGHT 24
CREATE_RENDERBUFFER buf2 WIDTH 24 HEIGHT 28
ASSERT_SIMILAR_EMD_HISTOGRAM RENDERBUFFERS buf1 buf2 TOLERANCE 1.0
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 3:49: height 28 of 'buf2' does not match height 24 of 'buf1' at "
      "3:44",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, BindSamplerBadSampler) {
  std::string program = R"(BIND_SAMPLER SAMPLER doesnotexist TEXTURE_UNIT 1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 1:22: 'doesnotexist' must be a sampler",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, BindStorageBufferBadStorageBuffer) {
  std::string program = R"(BIND_STORAGE_BUFFER BUFFER doesnotexist BINDING 1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 1:28: 'doesnotexist' must be a buffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, BindTextureBadTexture) {
  std::string program = R"(BIND_TEXTURE TEXTURE doesnotexist TEXTURE_UNIT 1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 1:22: 'doesnotexist' must be a texture",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, BindUniformBufferBadUniformBuffer) {
  std::string program = R"(BIND_UNIFORM_BUFFER BUFFER doesnotexist BINDING 1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 1:28: 'doesnotexist' must be a buffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, DumpRenderbufferBadRenderbuffer) {
  std::string program =
      R"(DUMP_RENDERBUFFER RENDERBUFFER doesnotexist FILE "temp.png"
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 1:32: 'doesnotexist' must be a renderbuffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, SetUniformBadProgram) {
  std::string program =
      R"(SET_UNIFORM PROGRAM prog LOCATION 1 TYPE float VALUES 0.1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 1:21: 'prog' must be a program",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, RunComputeNonexistentProgram) {
  std::string program =
      R"(RUN_COMPUTE PROGRAM prog NUM_GROUPS 1 1 1
)";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 1:21: 'prog' must be a program",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, RunComputeWithGraphicsProgram) {
  std::string program =
      R"(DECLARE_SHADER frag FRAGMENT
void main() { }
END
DECLARE_SHADER vert VERTEX
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
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 10:21: 'prog' must be a compute program, not a graphics program",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, RunGraphicsNonexistentProgram) {
  std::string program =
      R"(DECLARE_SHADER frag FRAGMENT
#version 320 es
precision highp float;
layout(location = 0) out vec4 color;
void main() {
 color = vec4(1.0, 0.0, 0.0, 1.0);
}
END

DECLARE_SHADER vert VERTEX
#version 320 es
layout(location = 0) in vec2 pos;
void main(void) {
    gl_Position = vec4(pos, 0.0, 1.0);
}
END

COMPILE_SHADER frag_compiled SHADER frag

COMPILE_SHADER vert_compiled SHADER vert

CREATE_BUFFER vertex_buffer SIZE_BYTES 24 INIT_TYPE float INIT_VALUES
                           0.0 -1.0
                           -1.0 1.0
                            1.0 1.0

CREATE_BUFFER index_buffer SIZE_BYTES 24 INIT_TYPE uint INIT_VALUES
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
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 33:11: 'nonexistent' must be a program",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, RunGraphicsNonexistentVertexBuffer) {
  std::string program =
      R"(DECLARE_SHADER frag FRAGMENT
#version 320 es
precision highp float;
layout(location = 0) out vec4 color;
void main() {
 color = vec4(1.0, 0.0, 0.0, 1.0);
}
END

DECLARE_SHADER vert VERTEX
#version 320 es
layout(location = 0) in vec2 pos;
void main(void) {
    gl_Position = vec4(pos, 0.0, 1.0);
}
END

COMPILE_SHADER frag_compiled SHADER frag

COMPILE_SHADER vert_compiled SHADER vert

CREATE_PROGRAM program SHADERS vert_compiled frag_compiled

CREATE_BUFFER index_buffer SIZE_BYTES 24 INIT_TYPE uint INIT_VALUES
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
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 32:19: vertex buffer 'nonexistent' must be a buffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, RunGraphicsNonexistentIndexBuffer) {
  std::string program =
      R"(DECLARE_SHADER frag FRAGMENT
#version 320 es
precision highp float;
layout(location = 0) out vec4 color;
void main() {
 color = vec4(1.0, 0.0, 0.0, 1.0);
}
END

DECLARE_SHADER vert VERTEX
#version 320 es
layout(location = 0) in vec2 pos;
void main(void) {
    gl_Position = vec4(pos, 0.0, 1.0);
}
END

COMPILE_SHADER frag_compiled SHADER frag

COMPILE_SHADER vert_compiled SHADER vert

CREATE_PROGRAM program SHADERS vert_compiled frag_compiled

CREATE_BUFFER vertex_buffer SIZE_BYTES 24 INIT_TYPE float INIT_VALUES
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
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 35:14: index buffer 'nonexistent' must be a buffer",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, RunGraphicsNonexistentFramebufferAttachment) {
  std::string program =
      R"(DECLARE_SHADER frag FRAGMENT
#version 320 es
precision highp float;
layout(location = 0) out vec4 color;
void main() {
 color = vec4(1.0, 0.0, 0.0, 1.0);
}
END

DECLARE_SHADER vert VERTEX
#version 320 es
layout(location = 0) in vec2 pos;
void main(void) {
    gl_Position = vec4(pos, 0.0, 1.0);
}
END

COMPILE_SHADER frag_compiled SHADER frag

COMPILE_SHADER vert_compiled SHADER vert

CREATE_PROGRAM program SHADERS vert_compiled frag_compiled

CREATE_BUFFER vertex_buffer SIZE_BYTES 24 INIT_TYPE float INIT_VALUES
                           0.0 -1.0
                           -1.0 1.0
                            1.0 1.0

CREATE_BUFFER index_buffer SIZE_BYTES 24 INIT_TYPE uint INIT_VALUES
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
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 40:12: framebuffer attachment 'nonexistent' must be a "
      "renderbuffer or texture",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, RunGraphicsWithComputeProgram) {
  std::string program =
      R"(DECLARE_SHADER comp COMPUTE
#version 320 es
void main() { }
END

COMPILE_SHADER comp_compiled SHADER comp

CREATE_PROGRAM program SHADERS comp_compiled

CREATE_BUFFER vertex_buffer SIZE_BYTES 24 INIT_TYPE float INIT_VALUES
                           0.0 -1.0
                           -1.0 1.0
                            1.0 1.0

CREATE_BUFFER index_buffer SIZE_BYTES 24 INIT_TYPE uint INIT_VALUES
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
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "ERROR: 21:11: 'program' must be a graphics program, not a compute "
      "program",
      message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, SetSamplerParameterNonexistentSampler) {
  std::string program =
      R"(SET_SAMPLER_PARAMETER PARAMETER TEXTURE_MAG_FILTER VALUE LINEAR SAMPLER nonexistent
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 1:73: 'nonexistent' must be a sampler",
            message_consumer.GetMessageString(0));
}

TEST_F(CheckerTestFixture, SetTextureParameterNonexistentSampler) {
  std::string program =
      R"(SET_TEXTURE_PARAMETER VALUE NEAREST PARAMETER TEXTURE_MIN_FILTER TEXTURE nonexistent
)";
  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("ERROR: 1:74: 'nonexistent' must be a texture",
            message_consumer.GetMessageString(0));
}

}  // namespace
}  // namespace shadertrap
