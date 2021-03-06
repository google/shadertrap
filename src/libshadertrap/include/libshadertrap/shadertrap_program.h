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

#ifndef LIBSHADERTRAP_SHADERTRAP_PROGRAM_H
#define LIBSHADERTRAP_SHADERTRAP_PROGRAM_H

#include <cstddef>
#include <memory>
#include <vector>

#include "libshadertrap/api_version.h"
#include "libshadertrap/command.h"

namespace shadertrap {

class ShaderTrapProgram {
 public:
  explicit ShaderTrapProgram(ApiVersion api_version,
                             std::vector<std::unique_ptr<Command>> commands);

  size_t GetNumCommands() const { return commands_.size(); }

  Command* GetCommand(size_t index) const { return commands_[index].get(); }

  const ApiVersion& GetApiVersion() const { return api_version_; }

 private:
  ApiVersion api_version_;
  std::vector<std::unique_ptr<Command>> commands_;
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_SHADERTRAP_PROGRAM_H
