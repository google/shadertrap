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

#include <EGL/egl.h>
#include <glad/glad.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "libshadertrap/checker.h"
#include "libshadertrap/command_visitor.h"
#include "libshadertrap/compound_visitor.h"
#include "libshadertrap/executor.h"
#include "libshadertrap/glslang.h"
#include "libshadertrap/helpers.h"
#include "libshadertrap/make_unique.h"
#include "libshadertrap/message_consumer.h"
#include "libshadertrap/parser.h"
#include "libshadertrap/shadertrap_program.h"
#include "libshadertrap/token.h"

namespace {

class ConsoleMessageConsumer : public shadertrap::MessageConsumer {
  void Message(Severity severity, const shadertrap::Token* token,
               const std::string& message) override {
    switch (severity) {
      case MessageConsumer::Severity::kError:
        std::cerr << "ERROR";
        break;
      case MessageConsumer::Severity::kWarning:
        std::cerr << "WARNING";
        break;
    }
    std::cerr << " at ";
    if (token == nullptr) {
      std::cerr << "unknown location";
    } else {
      std::cerr << token->GetLocationString();
    }
    std::cerr << ": " << message << std::endl;
  }
};

std::vector<char> ReadFile(const std::string& input_file) {
  std::ifstream file(input_file);
  std::ostringstream stringstream;
  stringstream << file.rdbuf();
  const std::string& temp = stringstream.str();
  return std::vector<char>(temp.begin(), temp.end());
}

}  // namespace

int main(int argc, const char** argv) {
  std::vector<std::string> args(argv, argv + argc);
  if (args.size() != 2) {
    std::cerr << "Usage: " << args[0] + " SCRIPT" << std::endl;
    return 1;
  }

  auto char_data = ReadFile(args[1]);
  auto data = std::string(char_data.begin(), char_data.end());

  ConsoleMessageConsumer message_consumer;
  shadertrap::Parser parser(data, &message_consumer);
  if (!parser.Parse()) {
    return 1;
  }

  EGLDisplay display;
  EGLConfig config;
  EGLContext context;
  EGLSurface surface;

  const EGLint config_attribute_list[] = {// EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                                          EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
                                          EGL_RED_SIZE,     4,
                                          EGL_GREEN_SIZE,   4,
                                          EGL_BLUE_SIZE,    4,
                                          EGL_ALPHA_SIZE,   4,

                                          EGL_CONFORMANT,   EGL_OPENGL_ES3_BIT,
                                          EGL_DEPTH_SIZE,   16,
                                          EGL_NONE};

  const EGLint context_attrib_list[] = {EGL_CONTEXT_CLIENT_VERSION, 3,
                                        EGL_NONE};

  // TODO(afd): For offscreen rendering, do width and height matter?  If no,
  //  are there more sensible default values than these?  If yes, should they be
  //  controllable from the command line?
  const EGLint pbuffer_attrib_list[] = {EGL_WIDTH,
                                        256,
                                        EGL_HEIGHT,
                                        256,
                                        EGL_TEXTURE_FORMAT,
                                        EGL_NO_TEXTURE,
                                        EGL_TEXTURE_TARGET,
                                        EGL_NO_TEXTURE,
                                        EGL_LARGEST_PBUFFER,
                                        EGL_TRUE,
                                        EGL_NONE};

  display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  EGLint major;
  EGLint minor;

  if (eglInitialize(display, &major, &minor) == EGL_FALSE) {
    crash("%s", "eglInitialize failed.");
  }

  EGLint num_config;
  if (eglChooseConfig(display, config_attribute_list, &config, 1,
                      &num_config) == EGL_FALSE) {
    crash("%s", "eglChooseConfig failed.");
  }

  if (num_config != 1) {
    crash("%s", "eglChooseConfig did not return 1 config.");
  }

  context =
      eglCreateContext(display, config, EGL_NO_CONTEXT, context_attrib_list);
  if (context == EGL_NO_CONTEXT) {
    crash("eglCreateContext failed: %x", eglGetError());
  }

  surface = eglCreatePbufferSurface(display, config, pbuffer_attrib_list);
  if (surface == EGL_NO_SURFACE) {
    crash("eglCreatePbufferSurface failed: %x", eglGetError());
  }

  eglMakeCurrent(display, surface, surface, context);

  if (gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(eglGetProcAddress)) ==
      0) {
    crash("gladLoadGLES2Loader failed");
  }

  std::unique_ptr<shadertrap::ShaderTrapProgram> shadertrap_program =
      parser.GetParsedProgram();
  std::vector<std::unique_ptr<shadertrap::CommandVisitor>> temp;
  temp.push_back(
      shadertrap::MakeUnique<shadertrap::Checker>(&message_consumer));
  temp.push_back(
      shadertrap::MakeUnique<shadertrap::Executor>(&message_consumer));
  shadertrap::CompoundVisitor checker_and_executor(std::move(temp));
  ShInitialize();
  bool success = checker_and_executor.VisitCommands(shadertrap_program.get());
  ShFinalize();
  if (!success) {
    std::cerr << "Errors occurred during execution." << std::endl;
    return 1;
  }
  std::cerr << "SUCCESS!" << std::endl;
  return 0;
}
