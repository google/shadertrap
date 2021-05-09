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
#include "libshadertrap/gl_functions.h"
#include "libshadertrap/glslang.h"
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
    std::cerr << "eglInitialize failed." << std::endl;
    return 1;
  }

  EGLint num_config;
  if (eglChooseConfig(display, config_attribute_list, &config, 1,
                      &num_config) == EGL_FALSE) {
    std::cerr << "eglChooseConfig failed." << std::endl;
    return 1;
  }

  if (num_config != 1) {
    std::cerr << "ERROR: eglChooseConfig returned " << num_config
              << " configurations; exactly 1 configuration is required";
    return 1;
  }

  context =
      eglCreateContext(display, config, EGL_NO_CONTEXT, context_attrib_list);
  if (context == EGL_NO_CONTEXT) {
    std::cerr << "eglCreateContext failed." << std::endl;
    return 1;
  }

  surface = eglCreatePbufferSurface(display, config, pbuffer_attrib_list);
  if (surface == EGL_NO_SURFACE) {
    std::cerr << "eglCreatePbufferSurface failed." << std::endl;
    return 1;
  }

  eglMakeCurrent(display, surface, surface, context);

  if (gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(eglGetProcAddress)) ==
      0) {
    std::cerr << "gladLoadGLES2Loader failed." << std::endl;
    return 1;
  }

  shadertrap::GlFunctions functions{};
  functions.glActiveShaderProgram_ = glActiveShaderProgram;
  functions.glActiveTexture_ = glActiveTexture;
  functions.glAttachShader_ = glAttachShader;
  functions.glBeginQuery_ = glBeginQuery;
  functions.glBeginTransformFeedback_ = glBeginTransformFeedback;
  functions.glBindAttribLocation_ = glBindAttribLocation;
  functions.glBindBuffer_ = glBindBuffer;
  functions.glBindBufferBase_ = glBindBufferBase;
  functions.glBindBufferRange_ = glBindBufferRange;
  functions.glBindFramebuffer_ = glBindFramebuffer;
  functions.glBindImageTexture_ = glBindImageTexture;
  functions.glBindProgramPipeline_ = glBindProgramPipeline;
  functions.glBindRenderbuffer_ = glBindRenderbuffer;
  functions.glBindSampler_ = glBindSampler;
  functions.glBindTexture_ = glBindTexture;
  functions.glBindTransformFeedback_ = glBindTransformFeedback;
  functions.glBindVertexArray_ = glBindVertexArray;
  functions.glBindVertexBuffer_ = glBindVertexBuffer;
  functions.glBlendBarrier_ = glBlendBarrier;
  functions.glBlendColor_ = glBlendColor;
  functions.glBlendEquation_ = glBlendEquation;
  functions.glBlendEquationSeparate_ = glBlendEquationSeparate;
  functions.glBlendEquationSeparatei_ = glBlendEquationSeparatei;
  functions.glBlendEquationi_ = glBlendEquationi;
  functions.glBlendFunc_ = glBlendFunc;
  functions.glBlendFuncSeparate_ = glBlendFuncSeparate;
  functions.glBlendFuncSeparatei_ = glBlendFuncSeparatei;
  functions.glBlendFunci_ = glBlendFunci;
  functions.glBlitFramebuffer_ = glBlitFramebuffer;
  functions.glBufferData_ = glBufferData;
  functions.glBufferSubData_ = glBufferSubData;
  functions.glCheckFramebufferStatus_ = glCheckFramebufferStatus;
  functions.glClear_ = glClear;
  functions.glClearBufferfi_ = glClearBufferfi;
  functions.glClearBufferfv_ = glClearBufferfv;
  functions.glClearBufferiv_ = glClearBufferiv;
  functions.glClearBufferuiv_ = glClearBufferuiv;
  functions.glClearColor_ = glClearColor;
  functions.glClearDepthf_ = glClearDepthf;
  functions.glClearStencil_ = glClearStencil;
  functions.glClientWaitSync_ = glClientWaitSync;
  functions.glColorMask_ = glColorMask;
  functions.glColorMaski_ = glColorMaski;
  functions.glCompileShader_ = glCompileShader;
  functions.glCompressedTexImage2D_ = glCompressedTexImage2D;
  functions.glCompressedTexImage3D_ = glCompressedTexImage3D;
  functions.glCompressedTexSubImage2D_ = glCompressedTexSubImage2D;
  functions.glCompressedTexSubImage3D_ = glCompressedTexSubImage3D;
  functions.glCopyBufferSubData_ = glCopyBufferSubData;
  functions.glCopyImageSubData_ = glCopyImageSubData;
  functions.glCopyTexImage2D_ = glCopyTexImage2D;
  functions.glCopyTexSubImage2D_ = glCopyTexSubImage2D;
  functions.glCopyTexSubImage3D_ = glCopyTexSubImage3D;
  functions.glCreateProgram_ = glCreateProgram;
  functions.glCreateShader_ = glCreateShader;
  functions.glCreateShaderProgramv_ = glCreateShaderProgramv;
  functions.glCullFace_ = glCullFace;
  functions.glDebugMessageCallback_ = glDebugMessageCallback;
  functions.glDebugMessageControl_ = glDebugMessageControl;
  functions.glDebugMessageInsert_ = glDebugMessageInsert;
  functions.glDeleteBuffers_ = glDeleteBuffers;
  functions.glDeleteFramebuffers_ = glDeleteFramebuffers;
  functions.glDeleteProgram_ = glDeleteProgram;
  functions.glDeleteProgramPipelines_ = glDeleteProgramPipelines;
  functions.glDeleteQueries_ = glDeleteQueries;
  functions.glDeleteRenderbuffers_ = glDeleteRenderbuffers;
  functions.glDeleteSamplers_ = glDeleteSamplers;
  functions.glDeleteShader_ = glDeleteShader;
  functions.glDeleteSync_ = glDeleteSync;
  functions.glDeleteTextures_ = glDeleteTextures;
  functions.glDeleteTransformFeedbacks_ = glDeleteTransformFeedbacks;
  functions.glDeleteVertexArrays_ = glDeleteVertexArrays;
  functions.glDepthFunc_ = glDepthFunc;
  functions.glDepthMask_ = glDepthMask;
  functions.glDepthRangef_ = glDepthRangef;
  functions.glDetachShader_ = glDetachShader;
  functions.glDisable_ = glDisable;
  functions.glDisableVertexAttribArray_ = glDisableVertexAttribArray;
  functions.glDisablei_ = glDisablei;
  functions.glDispatchCompute_ = glDispatchCompute;
  functions.glDispatchComputeIndirect_ = glDispatchComputeIndirect;
  functions.glDrawArrays_ = glDrawArrays;
  functions.glDrawArraysIndirect_ = glDrawArraysIndirect;
  functions.glDrawArraysInstanced_ = glDrawArraysInstanced;
  functions.glDrawBuffers_ = glDrawBuffers;
  functions.glDrawElements_ = glDrawElements;
  functions.glDrawElementsBaseVertex_ = glDrawElementsBaseVertex;
  functions.glDrawElementsIndirect_ = glDrawElementsIndirect;
  functions.glDrawElementsInstanced_ = glDrawElementsInstanced;
  functions.glDrawElementsInstancedBaseVertex_ = glDrawElementsInstancedBaseVertex;
  functions.glDrawRangeElements_ = glDrawRangeElements;
  functions.glDrawRangeElementsBaseVertex_ = glDrawRangeElementsBaseVertex;
  functions.glEnable_ = glEnable;
  functions.glEnableVertexAttribArray_ = glEnableVertexAttribArray;
  functions.glEnablei_ = glEnablei;
  functions.glEndQuery_ = glEndQuery;
  functions.glEndTransformFeedback_ = glEndTransformFeedback;
  functions.glFenceSync_ = glFenceSync;
  functions.glFinish_ = glFinish;
  functions.glFlush_ = glFlush;
  functions.glFlushMappedBufferRange_ = glFlushMappedBufferRange;
  functions.glFramebufferParameteri_ = glFramebufferParameteri;
  functions.glFramebufferRenderbuffer_ = glFramebufferRenderbuffer;
  functions.glFramebufferTexture_ = glFramebufferTexture;
  functions.glFramebufferTexture2D_ = glFramebufferTexture2D;
  functions.glFramebufferTextureLayer_ = glFramebufferTextureLayer;
  functions.glFrontFace_ = glFrontFace;
  functions.glGenBuffers_ = glGenBuffers;
  functions.glGenFramebuffers_ = glGenFramebuffers;
  functions.glGenProgramPipelines_ = glGenProgramPipelines;
  functions.glGenQueries_ = glGenQueries;
  functions.glGenRenderbuffers_ = glGenRenderbuffers;
  functions.glGenSamplers_ = glGenSamplers;
  functions.glGenTextures_ = glGenTextures;
  functions.glGenTransformFeedbacks_ = glGenTransformFeedbacks;
  functions.glGenVertexArrays_ = glGenVertexArrays;
  functions.glGenerateMipmap_ = glGenerateMipmap;
  functions.glGetActiveAttrib_ = glGetActiveAttrib;
  functions.glGetActiveUniform_ = glGetActiveUniform;
  functions.glGetActiveUniformBlockName_ = glGetActiveUniformBlockName;
  functions.glGetActiveUniformBlockiv_ = glGetActiveUniformBlockiv;
  functions.glGetActiveUniformsiv_ = glGetActiveUniformsiv;
  functions.glGetAttachedShaders_ = glGetAttachedShaders;
  functions.glGetAttribLocation_ = glGetAttribLocation;
  functions.glGetBooleani_v_ = glGetBooleani_v;
  functions.glGetBooleanv_ = glGetBooleanv;
  functions.glGetBufferParameteri64v_ = glGetBufferParameteri64v;
  functions.glGetBufferParameteriv_ = glGetBufferParameteriv;
  functions.glGetBufferPointerv_ = glGetBufferPointerv;
  functions.glGetDebugMessageLog_ = glGetDebugMessageLog;
  functions.glGetError_ = glGetError;
  functions.glGetFloatv_ = glGetFloatv;
  functions.glGetFragDataLocation_ = glGetFragDataLocation;
  functions.glGetFramebufferAttachmentParameteriv_ = glGetFramebufferAttachmentParameteriv;
  functions.glGetFramebufferParameteriv_ = glGetFramebufferParameteriv;
  functions.glGetGraphicsResetStatus_ = glGetGraphicsResetStatus;
  functions.glGetInteger64i_v_ = glGetInteger64i_v;
  functions.glGetInteger64v_ = glGetInteger64v;
  functions.glGetIntegeri_v_ = glGetIntegeri_v;
  functions.glGetIntegerv_ = glGetIntegerv;
  functions.glGetInternalformativ_ = glGetInternalformativ;
  functions.glGetMultisamplefv_ = glGetMultisamplefv;
  functions.glGetObjectLabel_ = glGetObjectLabel;
  functions.glGetObjectPtrLabel_ = glGetObjectPtrLabel;
  functions.glGetPointerv_ = glGetPointerv;
  functions.glGetProgramBinary_ = glGetProgramBinary;
  functions.glGetProgramInfoLog_ = glGetProgramInfoLog;
  functions.glGetProgramInterfaceiv_ = glGetProgramInterfaceiv;
  functions.glGetProgramPipelineInfoLog_ = glGetProgramPipelineInfoLog;
  functions.glGetProgramPipelineiv_ = glGetProgramPipelineiv;
  functions.glGetProgramResourceIndex_ = glGetProgramResourceIndex;
  functions.glGetProgramResourceLocation_ = glGetProgramResourceLocation;
  functions.glGetProgramResourceName_ = glGetProgramResourceName;
  functions.glGetProgramResourceiv_ = glGetProgramResourceiv;
  functions.glGetProgramiv_ = glGetProgramiv;
  functions.glGetQueryObjectuiv_ = glGetQueryObjectuiv;
  functions.glGetQueryiv_ = glGetQueryiv;
  functions.glGetRenderbufferParameteriv_ = glGetRenderbufferParameteriv;
  functions.glGetSamplerParameterIiv_ = glGetSamplerParameterIiv;
  functions.glGetSamplerParameterIuiv_ = glGetSamplerParameterIuiv;
  functions.glGetSamplerParameterfv_ = glGetSamplerParameterfv;
  functions.glGetSamplerParameteriv_ = glGetSamplerParameteriv;
  functions.glGetShaderInfoLog_ = glGetShaderInfoLog;
  functions.glGetShaderPrecisionFormat_ = glGetShaderPrecisionFormat;
  functions.glGetShaderSource_ = glGetShaderSource;
  functions.glGetShaderiv_ = glGetShaderiv;
  functions.glGetString_ = glGetString;
  functions.glGetStringi_ = glGetStringi;
  functions.glGetSynciv_ = glGetSynciv;
  functions.glGetTexLevelParameterfv_ = glGetTexLevelParameterfv;
  functions.glGetTexLevelParameteriv_ = glGetTexLevelParameteriv;
  functions.glGetTexParameterIiv_ = glGetTexParameterIiv;
  functions.glGetTexParameterIuiv_ = glGetTexParameterIuiv;
  functions.glGetTexParameterfv_ = glGetTexParameterfv;
  functions.glGetTexParameteriv_ = glGetTexParameteriv;
  functions.glGetTransformFeedbackVarying_ = glGetTransformFeedbackVarying;
  functions.glGetUniformBlockIndex_ = glGetUniformBlockIndex;
  functions.glGetUniformIndices_ = glGetUniformIndices;
  functions.glGetUniformLocation_ = glGetUniformLocation;
  functions.glGetUniformfv_ = glGetUniformfv;
  functions.glGetUniformiv_ = glGetUniformiv;
  functions.glGetUniformuiv_ = glGetUniformuiv;
  functions.glGetVertexAttribIiv_ = glGetVertexAttribIiv;
  functions.glGetVertexAttribIuiv_ = glGetVertexAttribIuiv;
  functions.glGetVertexAttribPointerv_ = glGetVertexAttribPointerv;
  functions.glGetVertexAttribfv_ = glGetVertexAttribfv;
  functions.glGetVertexAttribiv_ = glGetVertexAttribiv;
  functions.glGetnUniformfv_ = glGetnUniformfv;
  functions.glGetnUniformiv_ = glGetnUniformiv;
  functions.glGetnUniformuiv_ = glGetnUniformuiv;
  functions.glHint_ = glHint;
  functions.glInvalidateFramebuffer_ = glInvalidateFramebuffer;
  functions.glInvalidateSubFramebuffer_ = glInvalidateSubFramebuffer;
  functions.glIsBuffer_ = glIsBuffer;
  functions.glIsEnabled_ = glIsEnabled;
  functions.glIsEnabledi_ = glIsEnabledi;
  functions.glIsFramebuffer_ = glIsFramebuffer;
  functions.glIsProgram_ = glIsProgram;
  functions.glIsProgramPipeline_ = glIsProgramPipeline;
  functions.glIsQuery_ = glIsQuery;
  functions.glIsRenderbuffer_ = glIsRenderbuffer;
  functions.glIsSampler_ = glIsSampler;
  functions.glIsShader_ = glIsShader;
  functions.glIsSync_ = glIsSync;
  functions.glIsTexture_ = glIsTexture;
  functions.glIsTransformFeedback_ = glIsTransformFeedback;
  functions.glIsVertexArray_ = glIsVertexArray;
  functions.glLineWidth_ = glLineWidth;
  functions.glLinkProgram_ = glLinkProgram;
  functions.glMapBufferRange_ = glMapBufferRange;
  functions.glMemoryBarrier_ = glMemoryBarrier;
  functions.glMemoryBarrierByRegion_ = glMemoryBarrierByRegion;
  functions.glMinSampleShading_ = glMinSampleShading;
  functions.glObjectLabel_ = glObjectLabel;
  functions.glObjectPtrLabel_ = glObjectPtrLabel;
  functions.glPatchParameteri_ = glPatchParameteri;
  functions.glPauseTransformFeedback_ = glPauseTransformFeedback;
  functions.glPixelStorei_ = glPixelStorei;
  functions.glPolygonOffset_ = glPolygonOffset;
  functions.glPopDebugGroup_ = glPopDebugGroup;
  functions.glPrimitiveBoundingBox_ = glPrimitiveBoundingBox;
  functions.glProgramBinary_ = glProgramBinary;
  functions.glProgramParameteri_ = glProgramParameteri;
  functions.glProgramUniform1f_ = glProgramUniform1f;
  functions.glProgramUniform1fv_ = glProgramUniform1fv;
  functions.glProgramUniform1i_ = glProgramUniform1i;
  functions.glProgramUniform1iv_ = glProgramUniform1iv;
  functions.glProgramUniform1ui_ = glProgramUniform1ui;
  functions.glProgramUniform1uiv_ = glProgramUniform1uiv;
  functions.glProgramUniform2f_ = glProgramUniform2f;
  functions.glProgramUniform2fv_ = glProgramUniform2fv;
  functions.glProgramUniform2i_ = glProgramUniform2i;
  functions.glProgramUniform2iv_ = glProgramUniform2iv;
  functions.glProgramUniform2ui_ = glProgramUniform2ui;
  functions.glProgramUniform2uiv_ = glProgramUniform2uiv;
  functions.glProgramUniform3f_ = glProgramUniform3f;
  functions.glProgramUniform3fv_ = glProgramUniform3fv;
  functions.glProgramUniform3i_ = glProgramUniform3i;
  functions.glProgramUniform3iv_ = glProgramUniform3iv;
  functions.glProgramUniform3ui_ = glProgramUniform3ui;
  functions.glProgramUniform3uiv_ = glProgramUniform3uiv;
  functions.glProgramUniform4f_ = glProgramUniform4f;
  functions.glProgramUniform4fv_ = glProgramUniform4fv;
  functions.glProgramUniform4i_ = glProgramUniform4i;
  functions.glProgramUniform4iv_ = glProgramUniform4iv;
  functions.glProgramUniform4ui_ = glProgramUniform4ui;
  functions.glProgramUniform4uiv_ = glProgramUniform4uiv;
  functions.glProgramUniformMatrix2fv_ = glProgramUniformMatrix2fv;
  functions.glProgramUniformMatrix2x3fv_ = glProgramUniformMatrix2x3fv;
  functions.glProgramUniformMatrix2x4fv_ = glProgramUniformMatrix2x4fv;
  functions.glProgramUniformMatrix3fv_ = glProgramUniformMatrix3fv;
  functions.glProgramUniformMatrix3x2fv_ = glProgramUniformMatrix3x2fv;
  functions.glProgramUniformMatrix3x4fv_ = glProgramUniformMatrix3x4fv;
  functions.glProgramUniformMatrix4fv_ = glProgramUniformMatrix4fv;
  functions.glProgramUniformMatrix4x2fv_ = glProgramUniformMatrix4x2fv;
  functions.glProgramUniformMatrix4x3fv_ = glProgramUniformMatrix4x3fv;
  functions.glPushDebugGroup_ = glPushDebugGroup;
  functions.glReadBuffer_ = glReadBuffer;
  functions.glReadPixels_ = glReadPixels;
  functions.glReadnPixels_ = glReadnPixels;
  functions.glReleaseShaderCompiler_ = glReleaseShaderCompiler;
  functions.glRenderbufferStorage_ = glRenderbufferStorage;
  functions.glRenderbufferStorageMultisample_ = glRenderbufferStorageMultisample;
  functions.glResumeTransformFeedback_ = glResumeTransformFeedback;
  functions.glSampleCoverage_ = glSampleCoverage;
  functions.glSampleMaski_ = glSampleMaski;
  functions.glSamplerParameterIiv_ = glSamplerParameterIiv;
  functions.glSamplerParameterIuiv_ = glSamplerParameterIuiv;
  functions.glSamplerParameterf_ = glSamplerParameterf;
  functions.glSamplerParameterfv_ = glSamplerParameterfv;
  functions.glSamplerParameteri_ = glSamplerParameteri;
  functions.glSamplerParameteriv_ = glSamplerParameteriv;
  functions.glScissor_ = glScissor;
  functions.glShaderBinary_ = glShaderBinary;
  functions.glShaderSource_ = glShaderSource;
  functions.glStencilFunc_ = glStencilFunc;
  functions.glStencilFuncSeparate_ = glStencilFuncSeparate;
  functions.glStencilMask_ = glStencilMask;
  functions.glStencilMaskSeparate_ = glStencilMaskSeparate;
  functions.glStencilOp_ = glStencilOp;
  functions.glStencilOpSeparate_ = glStencilOpSeparate;
  functions.glTexBuffer_ = glTexBuffer;
  functions.glTexBufferRange_ = glTexBufferRange;
  functions.glTexImage2D_ = glTexImage2D;
  functions.glTexImage3D_ = glTexImage3D;
  functions.glTexParameterIiv_ = glTexParameterIiv;
  functions.glTexParameterIuiv_ = glTexParameterIuiv;
  functions.glTexParameterf_ = glTexParameterf;
  functions.glTexParameterfv_ = glTexParameterfv;
  functions.glTexParameteri_ = glTexParameteri;
  functions.glTexParameteriv_ = glTexParameteriv;
  functions.glTexStorage2D_ = glTexStorage2D;
  functions.glTexStorage2DMultisample_ = glTexStorage2DMultisample;
  functions.glTexStorage3D_ = glTexStorage3D;
  functions.glTexStorage3DMultisample_ = glTexStorage3DMultisample;
  functions.glTexSubImage2D_ = glTexSubImage2D;
  functions.glTexSubImage3D_ = glTexSubImage3D;
  functions.glTransformFeedbackVaryings_ = glTransformFeedbackVaryings;
  functions.glUniform1f_ = glUniform1f;
  functions.glUniform1fv_ = glUniform1fv;
  functions.glUniform1i_ = glUniform1i;
  functions.glUniform1iv_ = glUniform1iv;
  functions.glUniform1ui_ = glUniform1ui;
  functions.glUniform1uiv_ = glUniform1uiv;
  functions.glUniform2f_ = glUniform2f;
  functions.glUniform2fv_ = glUniform2fv;
  functions.glUniform2i_ = glUniform2i;
  functions.glUniform2iv_ = glUniform2iv;
  functions.glUniform2ui_ = glUniform2ui;
  functions.glUniform2uiv_ = glUniform2uiv;
  functions.glUniform3f_ = glUniform3f;
  functions.glUniform3fv_ = glUniform3fv;
  functions.glUniform3i_ = glUniform3i;
  functions.glUniform3iv_ = glUniform3iv;
  functions.glUniform3ui_ = glUniform3ui;
  functions.glUniform3uiv_ = glUniform3uiv;
  functions.glUniform4f_ = glUniform4f;
  functions.glUniform4fv_ = glUniform4fv;
  functions.glUniform4i_ = glUniform4i;
  functions.glUniform4iv_ = glUniform4iv;
  functions.glUniform4ui_ = glUniform4ui;
  functions.glUniform4uiv_ = glUniform4uiv;
  functions.glUniformBlockBinding_ = glUniformBlockBinding;
  functions.glUniformMatrix2fv_ = glUniformMatrix2fv;
  functions.glUniformMatrix2x3fv_ = glUniformMatrix2x3fv;
  functions.glUniformMatrix2x4fv_ = glUniformMatrix2x4fv;
  functions.glUniformMatrix3fv_ = glUniformMatrix3fv;
  functions.glUniformMatrix3x2fv_ = glUniformMatrix3x2fv;
  functions.glUniformMatrix3x4fv_ = glUniformMatrix3x4fv;
  functions.glUniformMatrix4fv_ = glUniformMatrix4fv;
  functions.glUniformMatrix4x2fv_ = glUniformMatrix4x2fv;
  functions.glUniformMatrix4x3fv_ = glUniformMatrix4x3fv;
  functions.glUnmapBuffer_ = glUnmapBuffer;
  functions.glUseProgram_ = glUseProgram;
  functions.glUseProgramStages_ = glUseProgramStages;
  functions.glValidateProgram_ = glValidateProgram;
  functions.glValidateProgramPipeline_ = glValidateProgramPipeline;
  functions.glVertexAttrib1f_ = glVertexAttrib1f;
  functions.glVertexAttrib1fv_ = glVertexAttrib1fv;
  functions.glVertexAttrib2f_ = glVertexAttrib2f;
  functions.glVertexAttrib2fv_ = glVertexAttrib2fv;
  functions.glVertexAttrib3f_ = glVertexAttrib3f;
  functions.glVertexAttrib3fv_ = glVertexAttrib3fv;
  functions.glVertexAttrib4f_ = glVertexAttrib4f;
  functions.glVertexAttrib4fv_ = glVertexAttrib4fv;
  functions.glVertexAttribBinding_ = glVertexAttribBinding;
  functions.glVertexAttribDivisor_ = glVertexAttribDivisor;
  functions.glVertexAttribFormat_ = glVertexAttribFormat;
  functions.glVertexAttribI4i_ = glVertexAttribI4i;
  functions.glVertexAttribI4iv_ = glVertexAttribI4iv;
  functions.glVertexAttribI4ui_ = glVertexAttribI4ui;
  functions.glVertexAttribI4uiv_ = glVertexAttribI4uiv;
  functions.glVertexAttribIFormat_ = glVertexAttribIFormat;
  functions.glVertexAttribIPointer_ = glVertexAttribIPointer;
  functions.glVertexAttribPointer_ = glVertexAttribPointer;
  functions.glVertexBindingDivisor_ = glVertexBindingDivisor;
  functions.glViewport_ = glViewport;
  functions.glWaitSync_ = glWaitSync;
  std::unique_ptr<shadertrap::ShaderTrapProgram> shadertrap_program =
      parser.GetParsedProgram();
  std::vector<std::unique_ptr<shadertrap::CommandVisitor>> temp;
  temp.push_back(
      shadertrap::MakeUnique<shadertrap::Checker>(&message_consumer));
  temp.push_back(
      shadertrap::MakeUnique<shadertrap::Executor>(&functions, &message_consumer));
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
