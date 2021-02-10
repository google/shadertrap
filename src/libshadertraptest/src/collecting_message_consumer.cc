// Copyright 2021 The ShaderTrap Project Authors
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

#include "libshadertraptest/collecting_message_consumer.h"

namespace shadertrap {

void CollectingMessageConsumer::Message(Severity severity, const Token* token,
                                        const std::string& message) {
  messages_.emplace_back(severity, token->GetLocationString() + ": " + message);
}

std::string CollectingMessageConsumer::GetMessageString(size_t index) {
  auto message = messages_.at(index);
  std::string result;
  switch (message.first) {
    case MessageConsumer::Severity::kWarning:
      result = "WARNING";
      break;
    case MessageConsumer::Severity::kError:
      result = "ERROR";
      break;
  }
  return std::string(result + ": " + message.second);
}

}  // namespace shadertrap
