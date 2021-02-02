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

#include "libshadertrap/parser.h"
#include "libshadertraptest/collecting_message_consumer.h"
#include "libshadertraptest/gtest.h"

namespace shadertrap {
namespace {

TEST(DeclareShader, RedeclareShader) {
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
  ASSERT_EQ("9:16: Identifier 's' already used at 1:16",
            message_consumer.GetMessageString(0));
}

TEST(CompileShader, UnknownShader) {
  std::string program = R"(COMPILE_SHADER result SHADER nonexistent
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1U, message_consumer.GetNumMessages());
  ASSERT_EQ(
      "1:30: Identifier 'nonexistent' does not correspond to a declared shader",
      message_consumer.GetMessageString(0));
}

TEST(CompileShader, NameAlreadyUsed) {
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
  ASSERT_EQ("9:16: Identifier 's' already used at 1:16",
            message_consumer.GetMessageString(0));
}

TEST(CreateBuffer, NameAlreadyUsed) {
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
  ASSERT_EQ("6:15: Identifier 'vert' already used at 1:16",
            message_consumer.GetMessageString(0));
}

TEST(CreateProgram, UnknownShader) {
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
      "14:43: Identifier 'mysampler' does not correspond to a compiled shader",
      message_consumer.GetMessageString(0));
}

TEST(CreateProgram, NameAlreadyUsed) {
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
  ASSERT_EQ("13:16: Identifier 'frag_compiled' already used at 12:16",
            message_consumer.GetMessageString(0));
}

TEST(CreateProgram, NoFragmentShader) {
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
  ASSERT_EQ("7:1: No fragment shader provided for 'CREATE_PROGRAM' command",
            message_consumer.GetMessageString(0));
}

TEST(CreateProgram, NoVertexShader) {
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
  ASSERT_EQ("7:1: No vertex shader provided for 'CREATE_PROGRAM' command",
            message_consumer.GetMessageString(0));
}

TEST(CreateProgram, MultipleFragmentShaders) {
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
      "13:57: Multiple fragment shaders provided to 'CREATE_PROGRAM'; "
      "already found 'frag_compiled' at 13:43",
      message_consumer.GetMessageString(0));
}

TEST(CreateProgram, MultipleVertexShaders) {
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
      "13:57: Multiple vertex shaders provided to 'CREATE_PROGRAM'; "
      "already found 'vert_compiled' at 13:43",
      message_consumer.GetMessageString(0));
}

TEST(CreateSampler, NameAlreadyUsed) {
  std::string program = R"(CREATE_EMPTY_TEXTURE_2D name WIDTH 12 HEIGHT 12
CREATE_SAMPLER name
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("2:16: Identifier 'name' already used at 1:25",
            message_consumer.GetMessageString(0));
}

TEST(CreateEmptyTexture2D, NameAlreadyUsed) {
  std::string program = R"(CREATE_EMPTY_TEXTURE_2D name WIDTH 12 HEIGHT 12
CREATE_EMPTY_TEXTURE_2D name WIDTH 12 HEIGHT 12
  )";

  CollectingMessageConsumer message_consumer;
  Parser parser(program, &message_consumer);
  ASSERT_TRUE(parser.Parse());
  Checker checker(&message_consumer);
  ASSERT_FALSE(checker.VisitCommands(parser.GetParsedProgram().get()));
  ASSERT_EQ(1, message_consumer.GetNumMessages());
  ASSERT_EQ("2:25: Identifier 'name' already used at 1:25",
            message_consumer.GetMessageString(0));
}

}  // namespace
}  // namespace shadertrap
