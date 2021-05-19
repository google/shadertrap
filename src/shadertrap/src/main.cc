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

#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "glad/glad.h"
#include "libshadertrap/api_version.h"
#include "libshadertrap/checker.h"
#include "libshadertrap/command_visitor.h"
#include "libshadertrap/compound_visitor.h"
#include "libshadertrap/executor.h"
#include "libshadertrap/gl_functions.h"
#include "libshadertrap/glslang.h"
#include "libshadertrap/make_unique.h"
#include "libshadertrap/message_consumer.h"
#include "libshadertrap/parser.h"
#include "libshadertrap/shadertrap_program.h"
#include "libshadertrap/token.h"
#include "shadertrap/get_gl_functions.h"

namespace {

const EGLint kWidth = 256;
const EGLint kHeight = 256;
const EGLint kDepthSize = 16;
const EGLint kRequiredEglMinorVersionForGl = 5;

const char* const kOptionPrefix = "--";
const char* const kOptionShowGlInfo = "--show-gl-info";

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

#define SHOW_GL_STRING(name)                                            \
  do {                                                                  \
    auto* gl_string = glGetString(name);                                \
    if (glGetError() != GL_NO_ERROR) {                                  \
      std::cerr << "Error calling glGetString(" #name ")" << std::endl; \
      return 1;                                                         \
    }                                                                   \
    std::cout << "GL_VENDOR: " << gl_string << std::endl;               \
  } while (false)

int main(int argc, const char** argv) {
  std::vector<std::string> args(argv, argv + argc);
  if (args.size() < 2) {
    std::cerr << "Usage: " << args[0] + "[options] SCRIPT" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  " << kOptionShowGlInfo << std::endl;
    std::cerr << "    Show GL information before running the script"
              << std::endl;
    return 1;
  }

  bool show_gl_info = false;
  std::string script_name;
  std::string option_prefix(kOptionPrefix);
  for (size_t i = 1; i < static_cast<size_t>(argc); i++) {
    std::string argument(argv[i]);
    if (argument == kOptionShowGlInfo) {
      show_gl_info = true;
    } else if (argument.length() >= option_prefix.length() &&
               argument.substr(0, option_prefix.length()) == option_prefix) {
      std::cerr << "Unknown option " << argument << std::endl;
      return 1;
    } else if (!script_name.empty()) {
      std::cerr << "Multiple script names provided." << std::endl;
      return 1;
    } else {
      script_name = argument;
    }
  }

  if (script_name.empty()) {
    std::cerr << "No script name was provided." << std::endl;
    return 1;
  }

  auto char_data = ReadFile(script_name);
  auto data = std::string(char_data.begin(), char_data.end());

  ConsoleMessageConsumer message_consumer;
  shadertrap::Parser parser(data, &message_consumer);
  if (!parser.Parse()) {
    return 1;
  }

  std::unique_ptr<shadertrap::ShaderTrapProgram> shadertrap_program =
      parser.GetParsedProgram();

  shadertrap::ApiVersion api_version = shadertrap_program->GetApiVersion();

  std::vector<EGLint> config_attributes = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
                                           EGL_RED_SIZE,     4,
                                           EGL_GREEN_SIZE,   4,
                                           EGL_BLUE_SIZE,    4,
                                           EGL_ALPHA_SIZE,   4,

                                           EGL_CONFORMANT,   EGL_OPENGL_ES3_BIT,
                                           EGL_DEPTH_SIZE,   kDepthSize,
                                           EGL_NONE};

  std::vector<EGLint> context_attributes = {
      EGL_CONTEXT_MAJOR_VERSION,
      static_cast<EGLint>(api_version.GetMajorVersion()),
      EGL_CONTEXT_MINOR_VERSION,
      static_cast<EGLint>(api_version.GetMinorVersion()), EGL_NONE};

  // TODO(afd): For offscreen rendering, do width and height matter?  If no,
  //  are there more sensible default values than these?  If yes, should they be
  //  controllable from the command line?
  std::vector<EGLint> pbuffer_attributes = {EGL_WIDTH,
                                            kWidth,
                                            EGL_HEIGHT,
                                            kHeight,
                                            EGL_TEXTURE_FORMAT,
                                            EGL_NO_TEXTURE,
                                            EGL_TEXTURE_TARGET,
                                            EGL_NO_TEXTURE,
                                            EGL_LARGEST_PBUFFER,
                                            EGL_TRUE,
                                            EGL_NONE};

  EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  EGLint egl_major_version;
  EGLint egl_minor_version;

  if (eglInitialize(display, &egl_major_version, &egl_minor_version) ==
      EGL_FALSE) {
    std::cerr << "Failed to initialize EGL display: ";
    switch (eglGetError()) {
      case EGL_BAD_DISPLAY:
        std::cerr << "EGL_BAD_DISPLAY";
        break;
      case EGL_NOT_INITIALIZED:
        std::cerr << "EGL_NOT_INITIALIZED";
        break;
      default:
        std::cerr << "unknown error";
        break;
    }
    std::cerr << std::endl;
    return 1;
  }

  if (api_version.GetApi() == shadertrap::ApiVersion::Api::GL &&
      !(egl_major_version > 1 ||
        (egl_major_version == 1 &&
         egl_minor_version >= kRequiredEglMinorVersionForGl))) {
    std::cerr << "EGL and OpenGL are not compatible pre EGL 1.5; found EGL "
              << egl_major_version << "." << egl_minor_version << std::endl;
    return 1;
  }

  if (eglBindAPI(static_cast<EGLenum>(api_version.GetApi() ==
                                              shadertrap::ApiVersion::Api::GL
                                          ? EGL_OPENGL_API
                                          : EGL_OPENGL_ES_API)) == EGL_FALSE) {
    std::cerr << "eglBindAPI failed." << std::endl;
  }

  EGLint num_config;
  EGLConfig config;
  if (eglChooseConfig(display, config_attributes.data(), &config, 1,
                      &num_config) == EGL_FALSE) {
    std::cerr << "eglChooseConfig failed." << std::endl;
    return 1;
  }

  if (num_config != 1) {
    std::cerr << "ERROR: eglChooseConfig returned " << num_config
              << " configurations; exactly 1 configuration is required";
    return 1;
  }

  EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT,
                             context_attributes.data());
  if (context == EGL_NO_CONTEXT) {
    std::cerr << "eglCreateContext failed." << std::endl;
    return 1;
  }

  EGLSurface surface = eglCreatePbufferSurface(display, config, pbuffer_attributes.data());
  if (surface == EGL_NO_SURFACE) {
    std::cerr << "eglCreatePbufferSurface failed." << std::endl;
    return 1;
  }

  eglMakeCurrent(display, surface, surface, context);

  if (api_version.GetApi() == shadertrap::ApiVersion::Api::GL) {
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(eglGetProcAddress)) ==
        0) {
      std::cerr << "gladLoadGLLoader failed." << std::endl;
      return 1;
    }
  } else {
    if (gladLoadGLES2Loader(
            reinterpret_cast<GLADloadproc>(eglGetProcAddress)) == 0) {
      std::cerr << "gladLoadGLES2Loader failed." << std::endl;
      return 1;
    }
  }

  if (show_gl_info) {
    SHOW_GL_STRING(GL_VENDOR);
    SHOW_GL_STRING(GL_RENDERER);
    SHOW_GL_STRING(GL_VERSION);
    SHOW_GL_STRING(GL_SHADING_LANGUAGE_VERSION);
  }

  shadertrap::GlFunctions functions = shadertrap::GetGlFunctions();

  std::vector<std::unique_ptr<shadertrap::CommandVisitor>> temp;
  temp.push_back(shadertrap::MakeUnique<shadertrap::Checker>(
      &message_consumer, shadertrap_program->GetApiVersion()));
  temp.push_back(shadertrap::MakeUnique<shadertrap::Executor>(
      &functions, &message_consumer, shadertrap_program->GetApiVersion()));
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
