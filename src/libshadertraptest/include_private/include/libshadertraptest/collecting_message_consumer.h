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

#ifndef LIBSHADERTRAPTEST_COLLECTING_MESSAGE_CONSUMER_H
#define LIBSHADERTRAPTEST_COLLECTING_MESSAGE_CONSUMER_H

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "libshadertrap/message_consumer.h"
#include "libshadertrap/token.h"

namespace shadertrap {

class CollectingMessageConsumer : public MessageConsumer {
 public:
  void Message(Severity severity, const Token* token,
               const std::string& message) override;

  size_t GetNumMessages() { return messages_.size(); }

  std::string GetMessageString(size_t index);

 private:
  std::vector<std::pair<Severity, std::string>> messages_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAPTEST_COLLECTING_MESSAGE_CONSUMER_H
