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

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <cassert>
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
const char* const kOptionRequiredVendorRendererSubstring =
    "--require-vendor-renderer-substring";
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

class EglData {
 public:
  explicit EglData(EGLDisplay display)
      : display_(display), context_(nullptr), surface_(nullptr) {}

  ~EglData() {
    if (surface_ != nullptr) {
      eglDestroySurface(display_, surface_);
    }
    if (context_ != nullptr) {
      eglDestroyContext(display_, context_);
    }
    if (display_ != nullptr) {
      eglTerminate(display_);
    }
  }

  EglData(const EglData&) = delete;
  EglData& operator=(const EglData&) = delete;

  EglData(EglData&&) = delete;
  EglData& operator=(EglData&&) = delete;

  EGLDisplay GetDisplay() {
    assert(display_ != nullptr && "Attempt to retrieve null display.");
    return display_;
  }

  void SetContext(EGLContext context) { context_ = context; }
  EGLContext GetContext() {
    assert(context_ != nullptr && "Attempt to retrieve null context.");
    return context_;
  }

  void SetSurface(EGLSurface surface) { surface_ = surface; }
  EGLSurface GetSurface() {
    assert(surface_ != nullptr && "Attempt to retrieve null surface.");
    return surface_;
  }

 private:
  EGLDisplay display_;
  EGLContext context_;
  EGLSurface surface_;
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
  if (args.size() < 2) {
    std::cerr << "Usage: " << args[0] + "[options] SCRIPT" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  " << kOptionRequiredVendorRendererSubstring << " string"
              << std::endl;
    std::cerr << "      Requires that at least one of the GL_VENDOR or "
                 "GL_RENDERER strings contain"
              << std::endl;
    std::cerr << "      the given string. This will skip any other usable "
                 "devices until a suitable"
              << std::endl;
    std::cerr << "      device is found." << std::endl;
    std::cerr << "  " << kOptionShowGlInfo << std::endl;
    std::cerr << "      Show GL information before running the script"
              << std::endl;
    return 1;
  }

  bool show_gl_info = false;
  std::string vendor_or_renderer_substring;
  std::string script_name;
  std::string option_prefix(kOptionPrefix);
  for (size_t i = 1; i < static_cast<size_t>(argc); i++) {
    std::string argument(argv[i]);
    if (argument == kOptionShowGlInfo) {
      show_gl_info = true;
    } else if (argument == kOptionRequiredVendorRendererSubstring) {
      if (!vendor_or_renderer_substring.empty()) {
        std::cerr << "Vendor/renderer substring specified multiple times."
                  << std::endl;
        return 1;
      }
      if (i == static_cast<size_t>(argc) - 1) {
        std::cerr << "No string specified for vendor/renderer substring."
                  << std::endl;
        return 1;
      }
      i++;
      vendor_or_renderer_substring = argv[i];
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

  auto eglQueryDevicesEXT = reinterpret_cast<PFNEGLQUERYDEVICESEXTPROC>(
      eglGetProcAddress("eglQueryDevicesEXT"));
  auto eglGetPlatformDisplayEXT =
      reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
          eglGetProcAddress("eglGetPlatformDisplayEXT"));

  bool extensions_available =
      eglQueryDevicesEXT != nullptr && eglGetPlatformDisplayEXT != nullptr;

  std::stringstream diagnostics;

  const int kMaxDevices = 16;
  std::vector<EGLDeviceEXT> egl_devices(kMaxDevices);
  EGLint num_devices;
  if (extensions_available) {
    eglQueryDevicesEXT(kMaxDevices, egl_devices.data(), &num_devices);
    if (num_devices == 0) {
      std::cerr << "No devices found." << std::endl;
      return 1;
    }
    diagnostics << "Number of devices found: " << num_devices << std::endl;
  } else {
    num_devices = 1;
    diagnostics << "Device-querying extensions are not available." << std::endl;
  }

  for (size_t i = 0; i < static_cast<size_t>(num_devices); i++) {
    diagnostics << std::endl << "Trying device " << i << std::endl;
    EglData egl_data(extensions_available
                         ? eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT,
                                                    egl_devices[i], nullptr)
                         : eglGetDisplay(EGL_DEFAULT_DISPLAY));
    EGLint egl_major_version;
    EGLint egl_minor_version;
    if (eglInitialize(egl_data.GetDisplay(), &egl_major_version,
                      &egl_minor_version) == EGL_FALSE) {
      diagnostics << "Failed to initialize EGL display " << i << ": ";
      switch (eglGetError()) {
        case EGL_BAD_DISPLAY:
          diagnostics << "EGL_BAD_DISPLAY";
          break;
        case EGL_NOT_INITIALIZED:
          diagnostics << "EGL_NOT_INITIALIZED";
          break;
        default:
          diagnostics << "unknown error";
          break;
      }
      diagnostics << std::endl;
      continue;
    }
    diagnostics << "Successfully initialized EGL using display " << i
                << std::endl;
    if (api_version.GetApi() == shadertrap::ApiVersion::Api::GL &&
        !(egl_major_version > 1 ||
          (egl_major_version == 1 &&
           egl_minor_version >= kRequiredEglMinorVersionForGl))) {
      diagnostics << "EGL and OpenGL are not compatible pre EGL 1.5; found EGL "
                  << egl_major_version << "." << egl_minor_version << std::endl;
      continue;
    }
    if (eglBindAPI(static_cast<EGLenum>(
            api_version.GetApi() == shadertrap::ApiVersion::Api::GL
                ? EGL_OPENGL_API
                : EGL_OPENGL_ES_API)) == EGL_FALSE) {
      diagnostics << "eglBindAPI failed." << std::endl;
      continue;
    }
    std::vector<EGLint> config_attributes = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RED_SIZE,     4,
        EGL_GREEN_SIZE,   4,
        EGL_BLUE_SIZE,    4,
        EGL_ALPHA_SIZE,   4,

        EGL_CONFORMANT,   EGL_OPENGL_ES3_BIT,
        EGL_DEPTH_SIZE,   kDepthSize,
        EGL_NONE};

    EGLint num_config;
    EGLConfig config;
    if (eglChooseConfig(egl_data.GetDisplay(), config_attributes.data(),
                        &config, 1, &num_config) == EGL_FALSE) {
      diagnostics << "eglChooseConfig failed." << std::endl;
      continue;
    }
    if (num_config != 1) {
      diagnostics << "ERROR: eglChooseConfig returned " << num_config
                  << " configurations; exactly 1 configuration is required";
      continue;
    }
    std::vector<EGLint> context_attributes = {
        EGL_CONTEXT_MAJOR_VERSION,
        static_cast<EGLint>(api_version.GetMajorVersion()),
        EGL_CONTEXT_MINOR_VERSION,
        static_cast<EGLint>(api_version.GetMinorVersion()), EGL_NONE};

    egl_data.SetContext(eglCreateContext(egl_data.GetDisplay(), config,
                                         EGL_NO_CONTEXT,
                                         context_attributes.data()));
    if (egl_data.GetContext() == EGL_NO_CONTEXT) {
      diagnostics << "eglCreateContext failed." << std::endl;
      continue;
    }

    // TODO(afd): For offscreen rendering, do width and height matter?  If no,
    //  are there more sensible default values than these?  If yes, should they
    //  be controllable from the command line?
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

    egl_data.SetSurface(eglCreatePbufferSurface(egl_data.GetDisplay(), config,
                                                pbuffer_attributes.data()));
    if (egl_data.GetSurface() == EGL_NO_SURFACE) {
      diagnostics << "eglCreatePbufferSurface failed." << std::endl;
      continue;
    }

    if (eglMakeCurrent(egl_data.GetDisplay(), egl_data.GetSurface(),
                       egl_data.GetSurface(),
                       egl_data.GetContext()) == EGL_FALSE) {
      diagnostics << "eglMakeCurrent failed." << std::endl;
      continue;
    }

    if (api_version.GetApi() == shadertrap::ApiVersion::Api::GL) {
      if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(eglGetProcAddress)) ==
          0) {
        diagnostics << "gladLoadGLLoader failed." << std::endl;
        continue;
      }
    } else {
      if (gladLoadGLES2Loader(
              reinterpret_cast<GLADloadproc>(eglGetProcAddress)) == 0) {
        diagnostics << "gladLoadGLES2Loader failed." << std::endl;
        continue;
      }
    }

    std::string gl_vendor(
        reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    if (glGetError() != GL_NO_ERROR) {
      diagnostics << "Error calling glGetString(GL_VENDOR)" << std::endl;
      continue;
    }
    std::string gl_renderer(
        reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    if (glGetError() != GL_NO_ERROR) {
      diagnostics << "Error calling glGetString(GL_RENDERER)" << std::endl;
      continue;
    }
    std::string gl_version(
        reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    if (glGetError() != GL_NO_ERROR) {
      diagnostics << "Error calling glGetString(GL_VERSION)" << std::endl;
      continue;
    }
    std::string gl_shading_language_version(reinterpret_cast<const char*>(
        glGetString(GL_SHADING_LANGUAGE_VERSION)));
    if (glGetError() != GL_NO_ERROR) {
      diagnostics << "Error calling glGetString(GL_SHADING_LANGUAGE_VERSION)"
                  << std::endl;
      continue;
    }

    if (gl_vendor.find(vendor_or_renderer_substring) == std::string::npos &&
        gl_renderer.find(vendor_or_renderer_substring) == std::string::npos) {
      diagnostics << "Skipping this device as it does not match the required "
                     "vendor/renderer substring "
                  << vendor_or_renderer_substring
                  << "; here is the GL info:" << std::endl;
      diagnostics << "GL_VENDOR: " + gl_vendor << std::endl;
      diagnostics << "GL_RENDERER: " + gl_renderer << std::endl;
      diagnostics << "GL_VERSION: " + gl_version << std::endl;
      diagnostics << "GL_SHADING_LANGUAGE_VERSION: " +
                         gl_shading_language_version
                  << std::endl;
      continue;
    }

    if (show_gl_info) {
      std::cout << "GL_VENDOR: " + gl_vendor << std::endl;
      std::cout << "GL_RENDERER: " + gl_renderer << std::endl;
      std::cout << "GL_VERSION: " + gl_version << std::endl;
      std::cout << "GL_SHADING_LANGUAGE_VERSION: " + gl_shading_language_version
                << std::endl;
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
  std::cerr << "It was not possible to find a suitable platform on which to "
               "run the script."
            << std::endl;
  std::cerr << diagnostics.str();
  return 1;
}
