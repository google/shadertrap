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

#ifndef LIBSHADERTRAP_PARSER_H
#define LIBSHADERTRAP_PARSER_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "libshadertrap/command.h"
#include "libshadertrap/message_consumer.h"
#include "libshadertrap/shadertrap_program.h"
#include "libshadertrap/token.h"
#include "libshadertrap/uniform_value.h"
#include "libshadertrap/vertex_attribute_info.h"

namespace shadertrap {

class Tokenizer;

class Parser {
 public:
  Parser(const std::string& input, MessageConsumer* message_consumer);

  ~Parser();

  Parser(const Parser&) = delete;

  Parser& operator=(const Parser&) = delete;

  Parser(Parser&&) = delete;

  Parser& operator=(Parser&&) = delete;

  bool Parse();

  std::unique_ptr<ShaderTrapProgram> GetParsedProgram();

 private:
  bool ParseCommand();

  bool ParseParameters(
      const std::map<Token::Type, std::function<bool()>>& parameter_parsers);

  bool ParseCommandAssertEqual();

  bool ParseCommandAssertPixels();

  bool ParseCommandAssertSimilarEmdHistogram();

  bool ParseCommandBindSampler();

  bool ParseCommandBindStorageBuffer();

  bool ParseCommandBindTexture();

  bool ParseCommandBindUniformBuffer();

  bool ParseCommandCompileShader();

  bool ParseCommandCreateEmptyTexture2d();

  bool ParseCommandCreateBuffer();

  bool ParseCommandCreateProgram();

  bool ParseCommandCreateRenderbuffer();

  bool ParseCommandCreateSampler();

  bool ParseCommandDeclareShader();

  bool ParseCommandDumpRenderbuffer();

  bool ParseCommandRunCompute();

  bool ParseCommandRunGraphics();

  bool ParseCommandSetTextureOrSamplerParameter();

  bool ParseCommandSetUniform();

  std::pair<bool, UniformValue> ProcessUniformValue(
      UniformValue::ElementType type,
      const std::pair<bool, size_t>& maybe_array_size,
      const std::vector<std::unique_ptr<Token>>& values);

  std::pair<bool, uint8_t> ParseUint8(const std::string& result_name);

  std::pair<bool, uint32_t> ParseUint32(const std::string& result_name);

  std::pair<bool, float> ParseFloat(const std::string& result_name);

  std::pair<bool, VertexAttributeInfo> ParseVertexAttributeInfo();

  std::unique_ptr<Tokenizer> tokenizer_;

  MessageConsumer* message_consumer_;

  std::vector<std::unique_ptr<Command>> parsed_commands_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_PARSER_H
