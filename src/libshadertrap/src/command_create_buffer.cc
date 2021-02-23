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

#include "libshadertrap/command_create_buffer.h"

#include <cstring>
#include <numeric>
#include <utility>

#include "libshadertrap/command_visitor.h"

namespace shadertrap {

CommandCreateBuffer::CommandCreateBuffer(
    std::unique_ptr<Token> start_token,
    std::unique_ptr<Token> result_identifier,
    const std::vector<ValuesSegment>& values)
    : Command(std::move(start_token)),
      result_identifier_(std::move(result_identifier)) {
  size_t size_bytes =
      std::accumulate(values.begin(), values.end(), size_t{0U},
                      [](size_t a, const ValuesSegment& segment) {
                        return a + segment.GetSizeBytes();
                      });
  data_.resize(size_bytes);
  size_t offset = 0;
  for (const auto& segment : values) {
    memcpy(data_.data() + offset, segment.GetData().data(),
           segment.GetSizeBytes());
    offset += segment.GetSizeBytes();
  }
}

bool CommandCreateBuffer::Accept(CommandVisitor* visitor) {
  return visitor->VisitCreateBuffer(this);
}

}  // namespace shadertrap
