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

TEST_F(CheckerTestFixture, AssertEqualDifferentSizedRenderbuffers) { FAIL(); }

TEST_F(CheckerTestFixture, AssertEqualDifferentSizedBuffers) { FAIL(); }

TEST_F(CheckerTestFixture, AssertEqualBufferVsRenderbuffer) { FAIL(); }

TEST_F(CheckerTestFixture, AssertEqualBadFistArgument) { FAIL(); }

TEST_F(CheckerTestFixture, AssertEqualBadSecondArgument) { FAIL(); }

TEST_F(CheckerTestFixture, AssertPixelsNotRenderbuffer) { FAIL(); }

TEST_F(CheckerTestFixture, AssertPixelsOutOfRangeX) { FAIL(); }

TEST_F(CheckerTestFixture, AssertPixelsOutOfRangeY) { FAIL(); }

TEST_F(CheckerTestFixture, TODO) { FAIL(); }

TEST_F(CheckerTestFixture, TODO) { FAIL(); }

}  // namespace
}  // namespace shadertrap
