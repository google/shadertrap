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

#include "libshadertrap/executor.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "libshadertrap/make_unique.h"
#include "libshadertrap/texture_parameter.h"
#include "libshadertrap/token.h"
#include "libshadertrap/uniform_value.h"
#include "libshadertrap/vertex_attribute_info.h"
#ifdef SHADERTRAP_LODEPNG
#include "lodepng/lodepng.h"
#endif

namespace shadertrap {

namespace {

const size_t kNumRgbaChannels = 4;

std::string OpenglErrorString(GLenum err) {
  switch (err) {
    case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";
    case GL_STACK_OVERFLOW:
      return "GL_STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW:
      return "GL_STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";
    default:
      return "UNKNOW_ERROR";
  }
}

template <typename T>
void DumpFormatEntry(const char* data,
                     const CommandDumpBufferText::FormatEntry& format_entry,
                     std::ofstream* text_file, size_t* index) {
  std::vector<T> values(format_entry.count);
  const size_t size_bytes = format_entry.count * sizeof(T);
  memcpy(values.data(), &data[*index], size_bytes);
  for (auto it = values.begin(); it != values.end(); it++) {
    *text_file << *it;
    if (it != values.end() - 1) {
      *text_file << " ";
    }
  }
  *index += size_bytes;
}

}  // namespace

#define GL_CHECKERR(token, function_name)                 \
  do {                                                    \
    GLenum __err = gl_functions_->glGetError_();          \
    if (__err != GL_NO_ERROR) {                           \
      message_consumer_->Message(                         \
          MessageConsumer::Severity::kError, token,       \
          "OpenGL error: " + std::string(function_name) + \
              "(): " + OpenglErrorString(__err));         \
      return false;                                       \
    }                                                     \
  } while (0)

#define GL_SAFECALL(token, function, ...)    \
  do {                                       \
    gl_functions_->function##_(__VA_ARGS__); \
    GL_CHECKERR(token, #function);           \
  } while (0)

#define GL_SAFECALL_NO_ARGS(token, function) \
  do {                                       \
    gl_functions_->function##_();            \
    GL_CHECKERR(token, #function);           \
  } while (0)

Executor::Executor(GlFunctions* gl_functions, MessageConsumer* message_consumer,
                   ApiVersion api_version)
    : gl_functions_(gl_functions),
      message_consumer_(message_consumer),
      api_version_(api_version) {}

bool Executor::VisitAssertEqual(CommandAssertEqual* assert_equal) {
  if (assert_equal->GetArgumentsAreRenderbuffers()) {
    return CheckEqualRenderbuffers(assert_equal);
  }
  return CheckEqualBuffers(assert_equal);
}

bool Executor::VisitAssertPixels(CommandAssertPixels* assert_pixels) {
  GLuint framebuffer_object_id;
  GL_SAFECALL(&assert_pixels->GetStartToken(), glGenFramebuffers, 1,
              &framebuffer_object_id);
  GL_SAFECALL(&assert_pixels->GetStartToken(), glBindFramebuffer,
              GL_FRAMEBUFFER, framebuffer_object_id);
  GL_SAFECALL(
      &assert_pixels->GetStartToken(), glFramebufferRenderbuffer,
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
      created_renderbuffers_.at(assert_pixels->GetRenderbufferIdentifier()));
  size_t width;
  size_t height;
  {
    GLint temp_width;
    GL_SAFECALL(&assert_pixels->GetStartToken(), glGetRenderbufferParameteriv,
                GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &temp_width);
    GLint temp_height;
    GL_SAFECALL(&assert_pixels->GetStartToken(), glGetRenderbufferParameteriv,
                GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &temp_height);
    width = static_cast<size_t>(temp_width);
    height = static_cast<size_t>(temp_height);
  }

  GLenum status = gl_functions_->glCheckFramebufferStatus_(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, &assert_pixels->GetStartToken(),
        "Incomplete framebuffer found for 'ASSERT_PIXELS' command; "
        "glCheckFramebufferStatus returned status " +
            std::to_string(status));
    return false;
  }

  std::vector<std::uint8_t> data(width * height * kNumRgbaChannels);
  if (api_version_.GetApi() == ApiVersion::Api::GL ||
      api_version_ >= ApiVersion(ApiVersion::Api::GLES, 3, 0)) {
    // OpenGL ES did not support glReadBuffer before 3.0, and reads will always
    // occur from color attachment 0 in OpenGL ES 2.0. Where the facility to
    // specify a read buffer is available, we explicitly specify that we would
    // like color attachment 0.
    GL_SAFECALL(&assert_pixels->GetStartToken(), glReadBuffer,
                GL_COLOR_ATTACHMENT0);
  }
  GL_SAFECALL(&assert_pixels->GetStartToken(), glReadPixels, 0, 0,
              static_cast<GLint>(width), static_cast<GLint>(height), GL_RGBA,
              GL_UNSIGNED_BYTE, data.data());
  bool result = true;
  for (size_t y = assert_pixels->GetRectangleY();
       y < assert_pixels->GetRectangleY() + assert_pixels->GetRectangleHeight();
       y++) {
    for (size_t x = assert_pixels->GetRectangleX();
         x <
         assert_pixels->GetRectangleX() + assert_pixels->GetRectangleWidth();
         x++) {
      uint8_t* start_of_pixel = &data[(height - y - 1) * width * 4 + x * 4];
      uint8_t r = start_of_pixel[0];
      uint8_t g = start_of_pixel[1];
      uint8_t b = start_of_pixel[2];
      uint8_t a = start_of_pixel[3];
      if (assert_pixels->GetExpectedR() != r ||
          assert_pixels->GetExpectedG() != g ||
          assert_pixels->GetExpectedB() != b ||
          assert_pixels->GetExpectedA() != a) {
        std::stringstream stringstream;
        stringstream << "Expected pixel ("
                     << static_cast<uint32_t>(assert_pixels->GetExpectedR())
                     << ", "
                     << static_cast<uint32_t>(assert_pixels->GetExpectedG())
                     << ", "
                     << static_cast<uint32_t>(assert_pixels->GetExpectedB())
                     << ", "
                     << static_cast<uint32_t>(assert_pixels->GetExpectedA())
                     << "), got (" << static_cast<uint32_t>(r) << ", "
                     << static_cast<uint32_t>(g) << ", "
                     << static_cast<uint32_t>(b) << ", "
                     << static_cast<uint32_t>(a) << ") at "
                     << assert_pixels->GetRenderbufferIdentifier() << "[" << x
                     << "][" << y << "]";
        message_consumer_->Message(MessageConsumer::Severity::kError,
                                   &assert_pixels->GetStartToken(),
                                   stringstream.str());
        result = false;
      }
    }
  }
  return result;
}

bool Executor::VisitAssertSimilarEmdHistogram(
    CommandAssertSimilarEmdHistogram* assert_similar_emd_histogram) {
  GLuint renderbuffers[2];
  renderbuffers[0] = created_renderbuffers_.at(
      assert_similar_emd_histogram->GetRenderbufferIdentifier1());
  renderbuffers[1] = created_renderbuffers_.at(
      assert_similar_emd_histogram->GetRenderbufferIdentifier2());

  size_t width[2] = {0, 0};
  size_t height[2] = {0, 0};

  for (auto index : {0, 1}) {
    {
      GLint temp_width;
      GL_SAFECALL(&assert_similar_emd_histogram->GetStartToken(),
                  glBindRenderbuffer, GL_RENDERBUFFER, renderbuffers[index]);
      GL_SAFECALL(&assert_similar_emd_histogram->GetStartToken(),
                  glGetRenderbufferParameteriv, GL_RENDERBUFFER,
                  GL_RENDERBUFFER_WIDTH, &temp_width);
      width[index] = static_cast<size_t>(temp_width);
    }
    {
      GLint temp_height;
      GL_SAFECALL(&assert_similar_emd_histogram->GetStartToken(),
                  glGetRenderbufferParameteriv, GL_RENDERBUFFER,
                  GL_RENDERBUFFER_HEIGHT, &temp_height);
      height[index] = static_cast<size_t>(temp_height);
    }
  }

  if (width[0] != width[1]) {
    std::stringstream stringstream;
    stringstream << "The widths of "
                 << assert_similar_emd_histogram->GetRenderbufferIdentifier1()
                 << " and "
                 << assert_similar_emd_histogram->GetRenderbufferIdentifier2()
                 << " do not match: " << width[0] << " vs. " << width[1];
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               &assert_similar_emd_histogram->GetStartToken(),
                               stringstream.str());
    return false;
  }

  if (height[0] != height[1]) {
    std::stringstream stringstream;
    stringstream << "The heights of "
                 << assert_similar_emd_histogram->GetRenderbufferIdentifier1()
                 << " and "
                 << assert_similar_emd_histogram->GetRenderbufferIdentifier2()
                 << " do not match: " << height[0] << " vs. " << height[1];
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               &assert_similar_emd_histogram->GetStartToken(),
                               stringstream.str());
    return false;
  }

  GLuint framebuffer_object_id;
  GL_SAFECALL(&assert_similar_emd_histogram->GetStartToken(), glGenFramebuffers,
              1, &framebuffer_object_id);
  GL_SAFECALL(&assert_similar_emd_histogram->GetStartToken(), glBindFramebuffer,
              GL_FRAMEBUFFER, framebuffer_object_id);
  for (auto index : {0, 1}) {
    GL_SAFECALL(&assert_similar_emd_histogram->GetStartToken(),
                glFramebufferRenderbuffer, GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(index),
                GL_RENDERBUFFER, renderbuffers[index]);
  }
  GLenum status = gl_functions_->glCheckFramebufferStatus_(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &assert_similar_emd_histogram->GetStartToken(),
        "Incomplete framebuffer found for 'ASSERT_SIMILAR_EMD_HISTOGRAM' "
        "command; glCheckFramebufferStatus returned status " +
            std::to_string(status));
    return false;
  }

  std::vector<std::uint8_t> data[2];
  for (auto index : {0, 1}) {
    data[index].resize(width[index] * height[index] * kNumRgbaChannels);
    GL_SAFECALL(&assert_similar_emd_histogram->GetStartToken(), glReadBuffer,
                GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(index));
    GL_SAFECALL(&assert_similar_emd_histogram->GetStartToken(), glReadPixels, 0,
                0, static_cast<GLint>(width[index]),
                static_cast<GLint>(height[index]), GL_RGBA, GL_UNSIGNED_BYTE,
                data[index].data());
  }

  const size_t num_bins = 256;

  std::vector<std::vector<uint64_t>> histogram[2];
  for (auto index : {0, 1}) {
    for (size_t channel = 0; channel < 4; channel++) {
      histogram[index].emplace_back(std::vector<uint64_t>(num_bins, 0));
    }
    for (size_t y = 0; y < height[index]; y++) {
      for (size_t x = 0; x < width[index]; x++) {
        for (size_t channel = 0; channel < 4; channel++) {
          histogram[index][channel]
                   [data[index][y * width[index] * 4 + x * 4 + channel]]++;
        }
      }
    }
  }

  // Earth movers's distance: Calculate the minimal cost of moving "earth" to
  // transform the first histogram into the second, where each bin of the
  // histogram can be thought of as a column of units of earth. The cost is the
  // amount of earth moved times the distance carried (the distance is the
  // number of adjacent bins over which the earth is carried). Calculate this
  // using the cumulative difference of the bins, which works as long as both
  // histograms have the same amount of earth. Sum the absolute values of the
  // cumulative difference to get the final cost of how much (and how far) the
  // earth was moved.
  double max_emd = 0;

  for (size_t channel = 0; channel < 4; ++channel) {
    double diff_total = 0;
    double diff_accum = 0;

    for (size_t i = 0; i < num_bins; ++i) {
      double hist_normalized_0 = static_cast<double>(histogram[0][channel][i]) /
                                 static_cast<double>(width[0] * height[0]);
      double hist_normalized_1 = static_cast<double>(histogram[1][channel][i]) /
                                 static_cast<double>(width[1] * height[1]);
      diff_accum += hist_normalized_0 - hist_normalized_1;
      diff_total += std::fabs(diff_accum);
    }
    // Normalize to range 0..1
    double emd = diff_total / num_bins;
    max_emd = std::max(max_emd, emd);
  }

  if (max_emd >
      static_cast<double>(assert_similar_emd_histogram->GetTolerance())) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError,
        &assert_similar_emd_histogram->GetStartToken(),
        "Histogram EMD value of " + std::to_string(max_emd) +
            " is greater than tolerance of " +
            std::to_string(assert_similar_emd_histogram->GetTolerance()));
  }
  return true;
}

bool Executor::VisitBindSampler(CommandBindSampler* bind_sampler) {
  GL_SAFECALL(&bind_sampler->GetStartToken(), glBindSampler,
              static_cast<GLuint>(bind_sampler->GetTextureUnit()),
              created_samplers_.at(bind_sampler->GetSamplerIdentifier()));
  return true;
}

bool Executor::VisitBindShaderStorageBuffer(
    CommandBindShaderStorageBuffer* bind_shader_storage_buffer) {
  GL_SAFECALL(
      &bind_shader_storage_buffer->GetStartToken(), glBindBufferBase,
      GL_SHADER_STORAGE_BUFFER,
      static_cast<GLuint>(bind_shader_storage_buffer->GetBinding()),
      created_buffers_.at(bind_shader_storage_buffer->GetBufferIdentifier()));
  return true;
}

bool Executor::VisitBindTexture(CommandBindTexture* bind_texture) {
  GL_SAFECALL(
      &bind_texture->GetStartToken(), glActiveTexture,
      GL_TEXTURE0 + static_cast<GLenum>(bind_texture->GetTextureUnit()));
  GL_SAFECALL(&bind_texture->GetStartToken(), glBindTexture, GL_TEXTURE_2D,
              created_textures_.at(bind_texture->GetTextureIdentifier()));
  return true;
}

bool Executor::VisitBindUniformBuffer(
    CommandBindUniformBuffer* bind_uniform_buffer) {
  GL_SAFECALL(&bind_uniform_buffer->GetStartToken(), glBindBufferBase,
              GL_UNIFORM_BUFFER,
              static_cast<GLuint>(bind_uniform_buffer->GetBinding()),
              created_buffers_.at(bind_uniform_buffer->GetBufferIdentifier()));
  return true;
}

bool Executor::VisitCompileShader(CommandCompileShader* compile_shader) {
  assert(declared_shaders_.count(compile_shader->GetShaderIdentifier()) == 1 &&
         "Shader not declared.");
  assert(compiled_shaders_.count(compile_shader->GetResultIdentifier()) == 0 &&
         "Identifier already in use for compiled shader.");
  CommandDeclareShader* shader_declaration =
      declared_shaders_.at(compile_shader->GetShaderIdentifier());
  GLenum shader_kind = GL_NONE;
  switch (shader_declaration->GetKind()) {
    case CommandDeclareShader::Kind::VERTEX:
      shader_kind = GL_VERTEX_SHADER;
      break;
    case CommandDeclareShader::Kind::FRAGMENT:
      shader_kind = GL_FRAGMENT_SHADER;
      break;
    case CommandDeclareShader::Kind::COMPUTE:
      shader_kind = GL_COMPUTE_SHADER;
      break;
  }
  GLuint shader = gl_functions_->glCreateShader_(shader_kind);
  GL_CHECKERR(&compile_shader->GetStartToken(), "glCreateShader");
  const char* temp = shader_declaration->GetShaderText().c_str();
  GL_SAFECALL(&compile_shader->GetStartToken(), glShaderSource, shader, 1,
              &temp, nullptr);
  GL_SAFECALL(&compile_shader->GetStartToken(), glCompileShader, shader);
  GLint status = 0;
  GL_SAFECALL(&compile_shader->GetStartToken(), glGetShaderiv, shader,
              GL_COMPILE_STATUS, &status);
  if (status == 0) {
    std::string message = "Shader compilation failed";
    GLint info_log_length = 0;
    GL_SAFECALL(&compile_shader->GetStartToken(), glGetShaderiv, shader,
                GL_INFO_LOG_LENGTH, &info_log_length);
    // The length includes the NULL character
    std::vector<GLchar> error_log(static_cast<size_t>(info_log_length), 0);
    GL_SAFECALL(&compile_shader->GetStartToken(), glGetShaderInfoLog, shader,
                info_log_length, &info_log_length, &error_log[0]);
    if (info_log_length > 0) {
      message += ":\n" + std::string(error_log.data());
    } else {
      message += " (no details available)";
    }
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               &compile_shader->GetStartToken(), message);
    return false;
  }
  compiled_shaders_.insert({compile_shader->GetResultIdentifier(), shader});
  return true;
}

bool Executor::VisitCreateBuffer(CommandCreateBuffer* create_buffer) {
  GLuint buffer;
  GL_SAFECALL(&create_buffer->GetStartToken(), glGenBuffers, 1, &buffer);
  // We arbitrarily bind to the ARRAY_BUFFER target.
  GL_SAFECALL(&create_buffer->GetStartToken(), glBindBuffer, GL_ARRAY_BUFFER,
              buffer);
  GL_SAFECALL(&create_buffer->GetStartToken(), glBufferData, GL_ARRAY_BUFFER,
              static_cast<GLsizeiptr>(create_buffer->GetSizeBytes()),
              create_buffer->GetData().data(), GL_STREAM_DRAW);
  created_buffers_.insert({create_buffer->GetResultIdentifier(), buffer});
  return true;
}

bool Executor::VisitCreateSampler(CommandCreateSampler* create_sampler) {
  GLuint sampler;
  GL_SAFECALL(&create_sampler->GetStartToken(), glGenSamplers, 1, &sampler);
  created_samplers_.insert({create_sampler->GetResultIdentifier(), sampler});
  return true;
}

bool Executor::VisitCreateEmptyTexture2D(
    CommandCreateEmptyTexture2D* create_empty_texture_2d) {
  GLuint texture;
  GL_SAFECALL(&create_empty_texture_2d->GetStartToken(), glGenTextures, 1,
              &texture);
  GL_SAFECALL(&create_empty_texture_2d->GetStartToken(), glBindTexture,
              GL_TEXTURE_2D, texture);
  GL_SAFECALL(&create_empty_texture_2d->GetStartToken(), glTexImage2D,
              GL_TEXTURE_2D, 0, GL_RGBA,
              static_cast<GLsizei>(create_empty_texture_2d->GetWidth()),
              static_cast<GLsizei>(create_empty_texture_2d->GetHeight()), 0,
              GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  created_textures_.insert(
      {create_empty_texture_2d->GetResultIdentifier(), texture});
  return true;
}

bool Executor::VisitCreateProgram(CommandCreateProgram* create_program) {
  assert(created_programs_.count(create_program->GetResultIdentifier()) == 0 &&
         "Identifier already in use for created program.");
  GLuint program = gl_functions_->glCreateProgram_();
  GL_CHECKERR(&create_program->GetStartToken(), "glCreateProgram");
  if (program == 0) {
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               &create_program->GetStartToken(),
                               "glCreateProgram failed");
    return false;
  }
  for (size_t index = 0; index < create_program->GetNumCompiledShaders();
       index++) {
    assert(compiled_shaders_.count(
               create_program->GetCompiledShaderIdentifier(index)) == 1 &&
           "Compiled shader not found.");
    GL_SAFECALL(&create_program->GetStartToken(), glAttachShader, program,
                compiled_shaders_.at(
                    create_program->GetCompiledShaderIdentifier(index)));
  }
  GL_SAFECALL(&create_program->GetStartToken(), glLinkProgram, program);
  GLint status = 0;
  GL_SAFECALL(&create_program->GetStartToken(), glGetProgramiv, program,
              GL_LINK_STATUS, &status);
  if (status == 0) {
    std::string message = "Program linking failed";
    GLint info_log_length = 0;
    GL_SAFECALL(&create_program->GetStartToken(), glGetProgramiv, program,
                GL_INFO_LOG_LENGTH, &info_log_length);
    // The length includes the NULL character
    std::vector<GLchar> error_log(static_cast<size_t>(info_log_length), 0);
    GL_SAFECALL(&create_program->GetStartToken(), glGetProgramInfoLog, program,
                info_log_length, &info_log_length, &error_log[0]);
    if (info_log_length > 0) {
      message += ":\n" + std::string(error_log.data());
    } else {
      message += " (no details available)";
    }
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               &create_program->GetStartToken(), message);
    return false;
  }
  created_programs_.insert({create_program->GetResultIdentifier(), program});
  return true;
}

bool Executor::VisitCreateRenderbuffer(
    CommandCreateRenderbuffer* create_renderbuffer) {
  GLuint render_buffer;
  GL_SAFECALL(&create_renderbuffer->GetStartToken(), glGenRenderbuffers, 1,
              &render_buffer);
  GL_SAFECALL(&create_renderbuffer->GetStartToken(), glBindRenderbuffer,
              GL_RENDERBUFFER, render_buffer);

  GL_SAFECALL(&create_renderbuffer->GetStartToken(), glRenderbufferStorage,
              GL_RENDERBUFFER, GL_RGBA8,
              static_cast<GLsizei>(create_renderbuffer->GetWidth()),
              static_cast<GLsizei>(create_renderbuffer->GetHeight()));
  created_renderbuffers_.insert(
      {create_renderbuffer->GetResultIdentifier(), render_buffer});
  return true;
}

bool Executor::VisitDeclareShader(CommandDeclareShader* declare_shader) {
  assert(declared_shaders_.count(declare_shader->GetResultIdentifier()) == 0 &&
         "Shader with this name already declared.");
  declared_shaders_.insert(
      {declare_shader->GetResultIdentifier(), declare_shader});
  return true;
}

bool Executor::VisitDumpRenderbuffer(
    CommandDumpRenderbuffer* dump_renderbuffer) {
  GLuint framebuffer_object_id;
  GL_SAFECALL(&dump_renderbuffer->GetStartToken(), glGenFramebuffers, 1,
              &framebuffer_object_id);
  GL_SAFECALL(&dump_renderbuffer->GetStartToken(), glBindFramebuffer,
              GL_FRAMEBUFFER, framebuffer_object_id);
  GL_SAFECALL(&dump_renderbuffer->GetStartToken(), glFramebufferRenderbuffer,
              GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
              created_renderbuffers_.at(
                  dump_renderbuffer->GetRenderbufferIdentifier()));
  size_t width;
  size_t height;
  {
    GLint temp_width;
    GL_SAFECALL(&dump_renderbuffer->GetStartToken(),
                glGetRenderbufferParameteriv, GL_RENDERBUFFER,
                GL_RENDERBUFFER_WIDTH, &temp_width);
    GLint temp_height;
    GL_SAFECALL(&dump_renderbuffer->GetStartToken(),
                glGetRenderbufferParameteriv, GL_RENDERBUFFER,
                GL_RENDERBUFFER_HEIGHT, &temp_height);
    width = static_cast<size_t>(temp_width);
    height = static_cast<size_t>(temp_height);
  }

  GLenum status = gl_functions_->glCheckFramebufferStatus_(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, &dump_renderbuffer->GetStartToken(),
        "Incomplete framebuffer found for 'DUMP_RENDERBUFFER' command; "
        "glCheckFramebufferStatus returned status " +
            std::to_string(status));
    return false;
  }

  std::vector<std::uint8_t> data(width * height * kNumRgbaChannels);
  GL_SAFECALL(&dump_renderbuffer->GetStartToken(), glReadBuffer,
              GL_COLOR_ATTACHMENT0);
  GL_SAFECALL(&dump_renderbuffer->GetStartToken(), glReadPixels, 0, 0,
              static_cast<GLint>(width), static_cast<GLint>(height), GL_RGBA,
              GL_UNSIGNED_BYTE, data.data());
  std::vector<std::uint8_t> flipped_data(width * height * kNumRgbaChannels);
  for (size_t h = 0; h < height; h++) {
    for (size_t col = 0; col < width * kNumRgbaChannels; col++) {
      flipped_data[h * width * kNumRgbaChannels + col] =
          data[(height - h - 1) * width * kNumRgbaChannels + col];
    }
  }
#ifdef SHADERTRAP_LODEPNG
  unsigned png_error = lodepng::encode(
      dump_renderbuffer->GetFilename(), flipped_data,
      static_cast<unsigned int>(width), static_cast<unsigned int>(height));
  if (png_error != 0) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, &dump_renderbuffer->GetStartToken(),
        "Writing PNG data to '" + dump_renderbuffer->GetFilename() +
            "' failed");
  }
  GL_SAFECALL(&dump_renderbuffer->GetStartToken(), glDeleteFramebuffers, 1,
              &framebuffer_object_id);
#endif
  return true;
}

bool Executor::VisitDumpBufferBinary(
    CommandDumpBufferBinary* dump_buffer_binary) {
  GLuint buffer =
      created_buffers_.at(dump_buffer_binary->GetBufferIdentifier());
  GLint64 buffer_size;
  GL_SAFECALL(&dump_buffer_binary->GetStartToken(), glBindBuffer,
              GL_ARRAY_BUFFER, buffer);
  GL_SAFECALL(&dump_buffer_binary->GetStartToken(), glGetBufferParameteri64v,
              GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffer_size);
  const auto* mapped_buffer =
      static_cast<char*>(gl_functions_->glMapBufferRange_(
          GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(buffer_size),
          GL_MAP_READ_BIT));
  if (mapped_buffer == nullptr) {
    GL_CHECKERR(&dump_buffer_binary->GetStartToken(), "glMapBufferRange");
    return false;
  }
  std::ofstream binary_file(dump_buffer_binary->GetFilename(),
                            std::ios::out | std::ios::binary);
  binary_file.write(mapped_buffer, buffer_size);
  GL_SAFECALL(&dump_buffer_binary->GetStartToken(), glUnmapBuffer,
              GL_ARRAY_BUFFER);
  return true;
}

bool Executor::VisitDumpBufferText(CommandDumpBufferText* dump_buffer_text) {
  GLuint buffer = created_buffers_.at(dump_buffer_text->GetBufferIdentifier());
  GLint64 buffer_size;
  GL_SAFECALL(&dump_buffer_text->GetStartToken(), glBindBuffer, GL_ARRAY_BUFFER,
              buffer);
  GL_SAFECALL(&dump_buffer_text->GetStartToken(), glGetBufferParameteri64v,
              GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffer_size);
  const auto* mapped_buffer =
      static_cast<char*>(gl_functions_->glMapBufferRange_(
          GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(buffer_size),
          GL_MAP_READ_BIT));
  if (mapped_buffer == nullptr) {
    GL_CHECKERR(&dump_buffer_text->GetStartToken(), "glMapBufferRange");
    return false;
  }
  std::ofstream text_file(dump_buffer_text->GetFilename(), std::ios::out);
  size_t index = 0;
  for (const auto& format_entry : dump_buffer_text->GetFormatEntries()) {
    switch (format_entry.kind) {
      case CommandDumpBufferText::FormatEntry::Kind::kSkip:
        index += format_entry.count;
        break;
      case CommandDumpBufferText::FormatEntry::Kind::kString:
        text_file << format_entry.token->GetText();
        break;
      case CommandDumpBufferText::FormatEntry::Kind::kByte:
        DumpFormatEntry<uint8_t>(mapped_buffer, format_entry, &text_file,
                                 &index);
        break;
      case CommandDumpBufferText::FormatEntry::Kind::kInt:
        DumpFormatEntry<int32_t>(mapped_buffer, format_entry, &text_file,
                                 &index);
        break;
      case CommandDumpBufferText::FormatEntry::Kind::kUint:
        DumpFormatEntry<uint32_t>(mapped_buffer, format_entry, &text_file,
                                  &index);
        break;
      case CommandDumpBufferText::FormatEntry::Kind::kFloat:
        DumpFormatEntry<float>(mapped_buffer, format_entry, &text_file, &index);
        break;
    }
  }
  GL_SAFECALL(&dump_buffer_text->GetStartToken(), glUnmapBuffer,
              GL_ARRAY_BUFFER);
  return true;
}

bool Executor::VisitRunCompute(CommandRunCompute* run_compute) {
  GL_SAFECALL(&run_compute->GetStartToken(), glUseProgram,
              created_programs_.at(run_compute->GetProgramIdentifier()));

  GL_SAFECALL(&run_compute->GetStartToken(), glDispatchCompute,
              static_cast<GLuint>(run_compute->GetNumGroupsX()),
              static_cast<GLuint>(run_compute->GetNumGroupsY()),
              static_cast<GLuint>(run_compute->GetNumGroupsZ()));

  GL_SAFECALL_NO_ARGS(&run_compute->GetStartToken(), glFlush);

  // Issue a memory barrier to ensure that future commands will see the effects
  // of this compute operation.
  GL_SAFECALL(&run_compute->GetStartToken(), glMemoryBarrier,
              GL_ALL_BARRIER_BITS);

  return true;
}

bool Executor::VisitRunGraphics(CommandRunGraphics* run_graphics) {
  GLuint vao;
  GL_SAFECALL(&run_graphics->GetStartToken(), glGenVertexArrays, 1, &vao);
  GL_SAFECALL(&run_graphics->GetStartToken(), glBindVertexArray, vao);

  const auto& vertex_data = run_graphics->GetVertexData();
  for (const auto& entry : vertex_data) {
    GL_SAFECALL(&run_graphics->GetStartToken(), glBindBuffer, GL_ARRAY_BUFFER,
                created_buffers_.at(entry.second.GetBufferIdentifier()));
    GL_SAFECALL(&run_graphics->GetStartToken(), glEnableVertexAttribArray,
                static_cast<GLuint>(entry.first));
    GL_SAFECALL(&run_graphics->GetStartToken(), glVertexAttribPointer,
                static_cast<GLuint>(entry.first),
                static_cast<GLsizei>(entry.second.GetDimension()), GL_FLOAT,
                GL_FALSE, static_cast<GLsizei>(entry.second.GetStrideBytes()),
                reinterpret_cast<void*>(entry.second.GetOffsetBytes()));
  }

  GL_SAFECALL(&run_graphics->GetStartToken(), glUseProgram,
              created_programs_.at(run_graphics->GetProgramIdentifier()));

  GLuint framebuffer_object_id;
  GL_SAFECALL(&run_graphics->GetStartToken(), glGenFramebuffers, 1,
              &framebuffer_object_id);
  GL_SAFECALL(&run_graphics->GetStartToken(), glBindFramebuffer, GL_FRAMEBUFFER,
              framebuffer_object_id);

  const auto& framebuffer_attachments =
      run_graphics->GetFramebufferAttachments();
  assert(framebuffer_attachments.size() <= 32 && "Too many renderbuffers.");
  size_t max_location = 0;
  for (const auto& entry : framebuffer_attachments) {
    max_location = std::max(max_location, entry.first);
  }
  std::vector<GLenum> draw_buffers;
  for (size_t i = 0; i <= max_location; i++) {
    if (framebuffer_attachments.count(i) > 0) {
      GLenum color_attachment = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
      auto framebuffer_attachment = framebuffer_attachments.at(i)->GetText();
      if (created_renderbuffers_.count(framebuffer_attachment) != 0) {
        GL_SAFECALL(&run_graphics->GetStartToken(), glFramebufferRenderbuffer,
                    GL_FRAMEBUFFER, color_attachment, GL_RENDERBUFFER,
                    created_renderbuffers_.at(framebuffer_attachment));
      } else {
        GL_SAFECALL(&run_graphics->GetStartToken(), glFramebufferTexture2D,
                    GL_FRAMEBUFFER, color_attachment, GL_TEXTURE_2D,
                    created_textures_.at(framebuffer_attachment), 0);
      }
      draw_buffers.push_back(color_attachment);
    } else {
      draw_buffers.push_back(GL_NONE);
    }
  }

  GLenum status = gl_functions_->glCheckFramebufferStatus_(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    message_consumer_->Message(
        MessageConsumer::Severity::kError, &run_graphics->GetStartToken(),
        "Incomplete framebuffer found for 'RUN_GRAPHICS' comment; "
        "glCheckFramebufferStatus returned status " +
            std::to_string(status));
    return false;
  }

  if (api_version_ != ApiVersion(ApiVersion::Api::GLES, 2, 0)) {
    // glDrawBuffers is not available in OpenGL ES 2.0, but for this API version
    // only color attachment 0 may be used, and the checker enforces this. Thus
    // this call can be skipped.
    GL_SAFECALL(&run_graphics->GetStartToken(), glDrawBuffers,
                static_cast<GLsizei>(draw_buffers.size()), draw_buffers.data());
  }

  GL_SAFECALL(&run_graphics->GetStartToken(), glClearColor, 0.0F, 0.0F, 0.0F,
              1.0F);
  GL_SAFECALL(&run_graphics->GetStartToken(), glClear, GL_COLOR_BUFFER_BIT);

  GL_SAFECALL(
      &run_graphics->GetStartToken(), glBindBuffer, GL_ELEMENT_ARRAY_BUFFER,
      created_buffers_.at(run_graphics->GetIndexDataBufferIdentifier()));
  GLenum topology = GL_NONE;
  switch (run_graphics->GetTopology()) {
    case CommandRunGraphics::Topology::kTriangles:
      topology = GL_TRIANGLES;
      break;
  }
  GL_SAFECALL(&run_graphics->GetStartToken(), glDrawElements, topology,
              static_cast<GLsizei>(run_graphics->GetVertexCount()),
              GL_UNSIGNED_INT, reinterpret_cast<GLvoid*>(0));

  GL_SAFECALL_NO_ARGS(&run_graphics->GetStartToken(), glFlush);

  for (const auto& entry : run_graphics->GetVertexData()) {
    GL_SAFECALL(&run_graphics->GetStartToken(), glDisableVertexAttribArray,
                static_cast<GLuint>(entry.first));
  }

  GL_SAFECALL(&run_graphics->GetStartToken(), glBindVertexArray, 0);
  GL_SAFECALL(&run_graphics->GetStartToken(), glDeleteVertexArrays, 1, &vao);

  GL_SAFECALL(&run_graphics->GetStartToken(), glDeleteFramebuffers, 1,
              &framebuffer_object_id);
  return true;
}

bool Executor::VisitSetSamplerParameter(
    CommandSetSamplerParameter* set_sampler_parameter) {
  GLenum parameter = GL_NONE;
  switch (set_sampler_parameter->GetParameter()) {
    case TextureParameter::kMagFilter:
      parameter = GL_TEXTURE_MAG_FILTER;
      break;
    case TextureParameter::kMinFilter:
      parameter = GL_TEXTURE_MIN_FILTER;
      break;
  }
  GLint parameter_value = GL_NONE;
  switch (set_sampler_parameter->GetParameterValue()) {
    case TextureParameterValue::kNearest:
      parameter_value = GL_NEAREST;
      break;
    case TextureParameterValue::kLinear:
      parameter_value = GL_LINEAR;
      break;
  }
  assert(created_samplers_.count(
             set_sampler_parameter->GetSamplerIdentifier()) > 0 &&
         "Unknown sampler.");
  GL_SAFECALL(
      &set_sampler_parameter->GetStartToken(), glSamplerParameteri,
      created_samplers_.at(set_sampler_parameter->GetSamplerIdentifier()),
      parameter, parameter_value);
  return true;
}

bool Executor::VisitSetTextureParameter(
    CommandSetTextureParameter* set_texture_parameter) {
  GLenum parameter = GL_NONE;
  switch (set_texture_parameter->GetParameter()) {
    case TextureParameter::kMagFilter:
      parameter = GL_TEXTURE_MAG_FILTER;
      break;
    case TextureParameter::kMinFilter:
      parameter = GL_TEXTURE_MIN_FILTER;
      break;
  }
  GLint parameter_value = GL_NONE;
  switch (set_texture_parameter->GetParameterValue()) {
    case TextureParameterValue::kNearest:
      parameter_value = GL_NEAREST;
      break;
    case TextureParameterValue::kLinear:
      parameter_value = GL_LINEAR;
      break;
  }
  assert(created_textures_.count(
             set_texture_parameter->GetTextureIdentifier()) > 0 &&
         "Unknown texture.");
  GL_SAFECALL(
      &set_texture_parameter->GetStartToken(), glBindTexture, GL_TEXTURE_2D,
      created_textures_.at(set_texture_parameter->GetTextureIdentifier()));
  GL_SAFECALL(&set_texture_parameter->GetStartToken(), glTexParameteri,
              GL_TEXTURE_2D, parameter, parameter_value);
  return true;
}

bool Executor::VisitSetUniform(CommandSetUniform* set_uniform) {
  GLuint program = created_programs_.at(set_uniform->GetProgramIdentifier());
  GLint uniform_location;
  if (set_uniform->HasLocation()) {
    uniform_location = static_cast<GLint>(set_uniform->GetLocation());
  } else {
    uniform_location = gl_functions_->glGetUniformLocation_(
        program, set_uniform->GetName().c_str());
    GL_CHECKERR(&set_uniform->GetStartToken(), "glGetUniformLocation");
    if (uniform_location == -1) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, &set_uniform->GetNameToken(),
          "Program '" + set_uniform->GetProgramIdentifier() +
              "' does not have a uniform named '" + set_uniform->GetName() +
              "'");
      return false;
    }
  }
  const UniformValue& uniform_value = set_uniform->GetValue();
  switch (uniform_value.GetElementType()) {
    case UniformValue::ElementType::kFloat:
      if (uniform_value.IsArray()) {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform1fv, program,
                    uniform_location,
                    static_cast<GLint>(uniform_value.GetArraySize()),
                    uniform_value.GetFloatData());
      } else {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform1f, program,
                    uniform_location, uniform_value.GetFloatData()[0]);
      }
      break;
    case UniformValue::ElementType::kVec2:
      if (uniform_value.IsArray()) {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform2fv, program,
                    uniform_location,
                    static_cast<GLsizei>(uniform_value.GetArraySize()),
                    uniform_value.GetFloatData());
      } else {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform2f, program,
                    uniform_location, uniform_value.GetFloatData()[0],
                    uniform_value.GetFloatData()[1]);
      }
      break;
    case UniformValue::ElementType::kVec3:
      if (uniform_value.IsArray()) {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform3fv, program,
                    uniform_location,
                    static_cast<GLsizei>(uniform_value.GetArraySize()),
                    uniform_value.GetFloatData());
      } else {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform3f, program,
                    uniform_location, uniform_value.GetFloatData()[0],
                    uniform_value.GetFloatData()[1],
                    uniform_value.GetFloatData()[2]);
      }
      break;
    case UniformValue::ElementType::kVec4:
      if (uniform_value.IsArray()) {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform4fv, program,
                    uniform_location,
                    static_cast<GLsizei>(uniform_value.GetArraySize()),
                    uniform_value.GetFloatData());
      } else {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform4f, program,
                    uniform_location, uniform_value.GetFloatData()[0],
                    uniform_value.GetFloatData()[1],
                    uniform_value.GetFloatData()[2],
                    uniform_value.GetFloatData()[3]);
      }
      break;
    case UniformValue::ElementType::kInt:
    case UniformValue::ElementType::kSampler2d:
      if (uniform_value.IsArray()) {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform1iv, program,
                    uniform_location,
                    static_cast<GLsizei>(uniform_value.GetArraySize()),
                    uniform_value.GetIntData());
      } else {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform1i, program,
                    uniform_location, uniform_value.GetIntData()[0]);
      }
      break;
    case UniformValue::ElementType::kIvec2:
      if (uniform_value.IsArray()) {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform2iv, program,
                    uniform_location,
                    static_cast<GLsizei>(uniform_value.GetArraySize()),
                    uniform_value.GetIntData());
      } else {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform2i, program,
                    uniform_location, uniform_value.GetIntData()[0],
                    uniform_value.GetIntData()[1]);
      }
      break;
    case UniformValue::ElementType::kIvec3:
      if (uniform_value.IsArray()) {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform3iv, program,
                    uniform_location,
                    static_cast<GLsizei>(uniform_value.GetArraySize()),
                    uniform_value.GetIntData());
      } else {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform3i, program,
                    uniform_location, uniform_value.GetIntData()[0],
                    uniform_value.GetIntData()[1],
                    uniform_value.GetIntData()[2]);
      }
      break;
    case UniformValue::ElementType::kIvec4:
      if (uniform_value.IsArray()) {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform4iv, program,
                    uniform_location,
                    static_cast<GLsizei>(uniform_value.GetArraySize()),
                    uniform_value.GetIntData());
      } else {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform4i, program,
                    uniform_location, uniform_value.GetIntData()[0],
                    uniform_value.GetIntData()[1],
                    uniform_value.GetIntData()[2],
                    uniform_value.GetIntData()[3]);
      }
      break;
    case UniformValue::ElementType::kUint:
      if (uniform_value.IsArray()) {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform1uiv,
                    program, uniform_location,
                    static_cast<GLsizei>(uniform_value.GetArraySize()),
                    uniform_value.GetUintData());
      } else {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform1ui, program,
                    uniform_location, uniform_value.GetUintData()[0]);
      }
      break;
    case UniformValue::ElementType::kUvec2:
      if (uniform_value.IsArray()) {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform2uiv,
                    program, uniform_location,
                    static_cast<GLsizei>(uniform_value.GetArraySize()),
                    uniform_value.GetUintData());
      } else {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform2ui, program,
                    uniform_location, uniform_value.GetUintData()[0],
                    uniform_value.GetUintData()[1]);
      }
      break;
    case UniformValue::ElementType::kUvec3:
      if (uniform_value.IsArray()) {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform3uiv,
                    program, uniform_location,
                    static_cast<GLsizei>(uniform_value.GetArraySize()),
                    uniform_value.GetUintData());
      } else {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform3ui, program,
                    uniform_location, uniform_value.GetUintData()[0],
                    uniform_value.GetUintData()[1],
                    uniform_value.GetUintData()[2]);
      }
      break;
    case UniformValue::ElementType::kUvec4:
      if (uniform_value.IsArray()) {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform4uiv,
                    program, uniform_location,
                    static_cast<GLsizei>(uniform_value.GetArraySize()),
                    uniform_value.GetUintData());
      } else {
        GL_SAFECALL(&set_uniform->GetStartToken(), glProgramUniform4ui, program,
                    uniform_location, uniform_value.GetUintData()[0],
                    uniform_value.GetUintData()[1],
                    uniform_value.GetUintData()[2],
                    uniform_value.GetUintData()[3]);
      }
      break;
    default:
      assert(false && "Unhandled uniform type.");
      break;
  }
  return true;
}

bool Executor::CheckEqualRenderbuffers(CommandAssertEqual* assert_equal) {
  assert(assert_equal->GetArgumentsAreRenderbuffers() &&
         "Arguments must be renderbuffers");
  assert(created_renderbuffers_.count(assert_equal->GetArgumentIdentifier1()) !=
             0 &&
         "Expected a renderbuffer");
  assert(created_renderbuffers_.count(assert_equal->GetArgumentIdentifier2()) !=
             0 &&
         "Expected a renderbuffer");

  GLuint renderbuffers[2];
  renderbuffers[0] =
      created_renderbuffers_.at(assert_equal->GetArgumentIdentifier1());
  renderbuffers[1] =
      created_renderbuffers_.at(assert_equal->GetArgumentIdentifier2());

  size_t width[2] = {0, 0};
  size_t height[2] = {0, 0};

  for (auto index : {0, 1}) {
    {
      GLint temp_width;
      GL_SAFECALL(&assert_equal->GetStartToken(), glBindRenderbuffer,
                  GL_RENDERBUFFER, renderbuffers[index]);
      GL_SAFECALL(&assert_equal->GetStartToken(), glGetRenderbufferParameteriv,
                  GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &temp_width);
      width[index] = static_cast<size_t>(temp_width);
    }
    {
      GLint temp_height;
      GL_SAFECALL(&assert_equal->GetStartToken(), glGetRenderbufferParameteriv,
                  GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &temp_height);
      height[index] = static_cast<size_t>(temp_height);
    }
  }

  if (width[0] != width[1]) {
    std::stringstream stringstream;
    stringstream << "The widths of " << assert_equal->GetArgumentIdentifier1()
                 << " and " << assert_equal->GetArgumentIdentifier2()
                 << " do not match: " << width[0] << " vs. " << width[1];
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               &assert_equal->GetStartToken(),
                               stringstream.str());
    return false;
  }

  if (height[0] != height[1]) {
    std::stringstream stringstream;
    stringstream << "The heights of " << assert_equal->GetArgumentIdentifier1()
                 << " and " << assert_equal->GetArgumentIdentifier2()
                 << " do not match: " << height[0] << " vs. " << height[1];
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               &assert_equal->GetStartToken(),
                               stringstream.str());
    return false;
  }

  std::vector<std::uint8_t> data[2];
  for (auto index : {0, 1}) {
    GLuint framebuffer_object_id;
    GL_SAFECALL(&assert_equal->GetStartToken(), glGenFramebuffers, 1,
                &framebuffer_object_id);
    GL_SAFECALL(&assert_equal->GetStartToken(), glBindFramebuffer,
                GL_FRAMEBUFFER, framebuffer_object_id);
    GL_SAFECALL(&assert_equal->GetStartToken(), glFramebufferRenderbuffer,
                GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                renderbuffers[index]);
    GLenum status = gl_functions_->glCheckFramebufferStatus_(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      message_consumer_->Message(
          MessageConsumer::Severity::kError, &assert_equal->GetStartToken(),
          "Incomplete framebuffer found for 'ASSERT_EQUAL' command; "
          "glCheckFramebufferStatus returned status " +
              std::to_string(status));
      return false;
    }
    data[index].resize(width[index] * height[index] * kNumRgbaChannels);
    GL_SAFECALL(&assert_equal->GetStartToken(), glReadBuffer,
                GL_COLOR_ATTACHMENT0);
    GL_SAFECALL(&assert_equal->GetStartToken(), glReadPixels, 0, 0,
                static_cast<GLsizei>(width[index]),
                static_cast<GLsizei>(height[index]), GL_RGBA, GL_UNSIGNED_BYTE,
                data[index].data());
    GL_SAFECALL(&assert_equal->GetStartToken(), glDeleteFramebuffers, 1,
                &framebuffer_object_id);
  }

  bool result = true;
  for (size_t y = 0; y < static_cast<size_t>(height[0]); y++) {
    for (size_t x = 0; x < static_cast<size_t>(width[0]); x++) {
      size_t offset = (static_cast<size_t>(height[0]) - y - 1) *
                          static_cast<size_t>(width[0]) * 4 +
                      x * 4;
      bool all_match = true;
      for (size_t component = 0; component < 4; component++) {
        if (data[0][offset + component] != data[1][offset + component]) {
          all_match = false;
          break;
        }
      }
      if (!all_match) {
        std::stringstream stringstream;
        stringstream << "Pixel mismatch at position (" << x << ", " << y
                     << "): " << assert_equal->GetArgumentIdentifier1() << "["
                     << x << "][" << y << "] == ("
                     << static_cast<uint32_t>(data[0][offset]) << ", "
                     << static_cast<uint32_t>(data[0][offset + 1]) << ", "
                     << static_cast<uint32_t>(data[0][offset + 2]) << ", "
                     << static_cast<uint32_t>(data[0][offset + 3]) << "), vs. "
                     << assert_equal->GetArgumentIdentifier2() << "[" << x
                     << "][" << y << "] == ("
                     << static_cast<uint32_t>(data[1][offset]) << ", "
                     << static_cast<uint32_t>(data[1][offset + 1]) << ", "
                     << static_cast<uint32_t>(data[1][offset + 2]) << ", "
                     << static_cast<uint32_t>(data[1][offset + 3]) << ")";
        message_consumer_->Message(MessageConsumer::Severity::kError,
                                   &assert_equal->GetStartToken(),
                                   stringstream.str());
        result = false;
      }
    }
  }
  return result;
}

bool Executor::CheckEqualBuffers(CommandAssertEqual* assert_equal) {
  assert(!assert_equal->GetArgumentsAreRenderbuffers() &&
         "Arguments must be buffers");
  assert(created_buffers_.count(assert_equal->GetArgumentIdentifier1()) != 0 &&
         "Expected a buffer");
  assert(created_buffers_.count(assert_equal->GetArgumentIdentifier2()) != 0 &&
         "Expected a buffer");

  GLuint buffers[2];
  buffers[0] = created_buffers_.at(assert_equal->GetArgumentIdentifier1());
  buffers[1] = created_buffers_.at(assert_equal->GetArgumentIdentifier2());

  GLint64 buffer_size[2]{0, 0};
  for (auto index : {0, 1}) {
    GL_SAFECALL(&assert_equal->GetStartToken(), glBindBuffer, GL_ARRAY_BUFFER,
                buffers[index]);
    GL_SAFECALL(&assert_equal->GetStartToken(), glGetBufferParameteri64v,
                GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffer_size[index]);
  }

  if (buffer_size[0] != buffer_size[1]) {
    std::stringstream stringstream;
    stringstream << "The lengths of " << assert_equal->GetArgumentIdentifier1()
                 << " and " << assert_equal->GetArgumentIdentifier2()
                 << " do not match: " << buffer_size[0] << " vs. "
                 << buffer_size[1];
    message_consumer_->Message(MessageConsumer::Severity::kError,
                               &assert_equal->GetStartToken(),
                               stringstream.str());
    return false;
  }

  uint8_t* mapped_buffer[2]{nullptr, nullptr};
  for (auto index : {0, 1}) {
    GL_SAFECALL(&assert_equal->GetStartToken(), glBindBuffer, GL_ARRAY_BUFFER,
                buffers[index]);
    mapped_buffer[index] =
        static_cast<uint8_t*>(gl_functions_->glMapBufferRange_(
            GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(buffer_size[index]),
            GL_MAP_READ_BIT));
    if (mapped_buffer[index] == nullptr) {
      GL_CHECKERR(&assert_equal->GetStartToken(), "glMapBufferRange");
      return false;
    }
  }

  std::vector<CommandAssertEqual::FormatEntry>& format_entries =
      assert_equal->GetFormatEntries();

  if (format_entries.empty()) {
    // No format entries were specified, so a default byte-based format entry,
    // based on the size of the buffers, is used.
    const Token& start_token = assert_equal->GetStartToken();
    format_entries.push_back(
        {MakeUnique<Token>(start_token.GetType(), start_token.GetLine(), 0U),
         CommandAssertEqual::FormatEntry::Kind::kByte,
         static_cast<size_t>(buffer_size[0])});
  }

  bool result = true;
  size_t offset = 0;

  for (auto& format_entry : format_entries) {
    switch (format_entry.kind) {
      case CommandAssertEqual::FormatEntry::Kind::kSkip:
        offset += format_entry.count;
        break;
      case CommandAssertEqual::FormatEntry::Kind::kByte: {
        for (size_t index = offset; index < format_entry.count; index++) {
          // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
          uint8_t value_1 = mapped_buffer[0][index];
          // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
          uint8_t value_2 = mapped_buffer[1][index];
          if (value_1 != value_2) {
            std::stringstream stringstream;
            stringstream << "Byte mismatch at index " << index << ": "
                         << assert_equal->GetArgumentIdentifier1() << "["
                         << index << "] == " << static_cast<uint32_t>(value_1)
                         << ", " << assert_equal->GetArgumentIdentifier2()
                         << "[" << index
                         << "] == " << static_cast<uint32_t>(value_2);
            message_consumer_->Message(MessageConsumer::Severity::kError,
                                       &assert_equal->GetStartToken(),
                                       stringstream.str());
            result = false;
          }
        }
        offset += format_entry.count;
        break;
      }
      case CommandAssertEqual::FormatEntry::Kind::kFloat: {
        float* float_region[2]{nullptr, nullptr};
        for (auto index : {0, 1}) {
          float_region[index] =
              // cppcheck-suppress invalidPointerCast
              reinterpret_cast<float*>(mapped_buffer[index] + offset);
        }

        for (size_t index = 0; index < format_entry.count; index++) {
          if (float_region[0] != nullptr && float_region[1] != nullptr) {
            // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
            float value_1 = float_region[0][index];
            // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
            float value_2 = float_region[1][index];
            // We compare data using memcmp to look for byte-level mismatches,
            // and then report any mismatches at the floating-point level. This
            // avoids performing floating-point comparisons, and associated
            // issues related to special values.
            if (std::memcmp(&float_region[0][index], &float_region[1][index],
                            sizeof(float)) != 0) {
              std::stringstream stringstream;
              size_t float_index = sizeof(float) * index + offset;
              stringstream << "Float mismatch at byte index " << float_index
                           << ": " << assert_equal->GetArgumentIdentifier1()
                           << "[" << float_index
                           << "] == " << static_cast<float>(value_1) << ", "
                           << assert_equal->GetArgumentIdentifier2() << "["
                           << float_index
                           << "] == " << static_cast<float>(value_2);
              message_consumer_->Message(MessageConsumer::Severity::kError,
                                         &assert_equal->GetStartToken(),
                                         stringstream.str());
              result = false;
            }
          }
        }
        offset += format_entry.count * sizeof(float);
        break;
      }
      case CommandAssertEqual::FormatEntry::Kind::kInt: {
        int32_t* int_region[2]{nullptr, nullptr};
        for (auto index : {0, 1}) {
          int_region[index] =
              reinterpret_cast<int32_t*>(mapped_buffer[index] + offset);
        }

        for (size_t index = 0; index < format_entry.count; index++) {
          // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
          int32_t value_1 = int_region[0][index];
          // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
          int32_t value_2 = int_region[1][index];
          if (value_1 != value_2) {
            size_t int_index = index * sizeof(int32_t) + offset;
            std::stringstream stringstream;
            stringstream << "Integer mismatch at byte_index " << int_index
                         << ": " << assert_equal->GetArgumentIdentifier1()
                         << "[" << int_index
                         << "] == " << static_cast<int32_t>(value_1) << ", "
                         << assert_equal->GetArgumentIdentifier2() << "["
                         << int_index
                         << "] == " << static_cast<int32_t>(value_2);
            message_consumer_->Message(MessageConsumer::Severity::kError,
                                       &assert_equal->GetStartToken(),
                                       stringstream.str());
            result = false;
          }
        }
        offset += format_entry.count * sizeof(int32_t);
        break;
      }
      case CommandAssertEqual::FormatEntry::Kind::kUint: {
        uint32_t* uint_region[2]{nullptr, nullptr};
        for (auto index : {0, 1}) {
          uint_region[index] =
              reinterpret_cast<uint32_t*>(mapped_buffer[index] + offset);
        }

        for (size_t index = 0; index < format_entry.count; index++) {
          // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
          uint32_t value_1 = uint_region[0][index];
          // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
          uint32_t value_2 = uint_region[1][index];
          if (value_1 != value_2) {
            size_t uint_index = index * sizeof(uint32_t) + offset;
            std::stringstream stringstream;
            stringstream << "Unsigned integer mismatch at byte index "
                         << uint_index << ": "
                         << assert_equal->GetArgumentIdentifier1() << "["
                         << uint_index
                         << "] == " << static_cast<uint32_t>(value_1) << ", "
                         << assert_equal->GetArgumentIdentifier2() << "["
                         << uint_index
                         << "] == " << static_cast<uint32_t>(value_2);
            message_consumer_->Message(MessageConsumer::Severity::kError,
                                       &assert_equal->GetStartToken(),
                                       stringstream.str());
            result = false;
          }
        }
        offset += format_entry.count * sizeof(uint32_t);
        break;
      }
    }
  }

  for (auto index : {0, 1}) {
    GL_SAFECALL(&assert_equal->GetStartToken(), glBindBuffer, GL_ARRAY_BUFFER,
                buffers[index]);
    GL_SAFECALL(&assert_equal->GetStartToken(), glUnmapBuffer, GL_ARRAY_BUFFER);
  }
  return result;
}

}  // namespace shadertrap
