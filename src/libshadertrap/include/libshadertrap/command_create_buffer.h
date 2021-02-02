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

#ifndef LIBSHADERTRAP_COMMAND_CREATE_BUFFER_H
#define LIBSHADERTRAP_COMMAND_CREATE_BUFFER_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "libshadertrap/command.h"
#include "libshadertrap/token.h"

namespace shadertrap {

class CommandCreateBuffer : public Command {
 public:
  enum class InitialDataType { kByte, kFloat, kInt, kUint, kNone };

  CommandCreateBuffer(std::unique_ptr<Token> start_token,
                      std::unique_ptr<Token> result_identifier,
                      size_t size_bytes, const std::vector<uint8_t>& byte_data);

  CommandCreateBuffer(std::unique_ptr<Token> start_token,
                      std::unique_ptr<Token> result_identifier,
                      size_t size_bytes, const std::vector<float>& float_data);

  CommandCreateBuffer(std::unique_ptr<Token> start_token,
                      std::unique_ptr<Token> result_identifier,
                      size_t size_bytes, const std::vector<int32_t>& int_data);

  CommandCreateBuffer(std::unique_ptr<Token> start_token,
                      std::unique_ptr<Token> result_identifier,
                      size_t size_bytes,
                      const std::vector<uint32_t>& uint_data);

  bool Accept(CommandVisitor* visitor) override;

  const std::string& GetResultIdentifier() const {
    return result_identifier_->GetText();
  }

  const Token* GetResultIdentifierToken() const {
    return result_identifier_.get();
  }

  size_t GetSizeBytes() const { return size_bytes_; }

  bool HasInitialData() const { return has_initial_data_; }

  const std::vector<uint8_t>& GetInitialData() const { return initial_data_; }

  InitialDataType GetInitialDataType() const { return initial_data_type_; }

 private:
  std::unique_ptr<Token> result_identifier_;
  size_t size_bytes_;
  bool has_initial_data_;
  std::vector<uint8_t> initial_data_;
  InitialDataType initial_data_type_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_COMMAND_CREATE_BUFFER_H
