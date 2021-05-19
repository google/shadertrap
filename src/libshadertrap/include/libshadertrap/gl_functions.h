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

// Automatically-generated file - DO NOT EDIT

#ifndef LIBSHADERTRAP_GL_FUNCTIONS_H
#define LIBSHADERTRAP_GL_FUNCTIONS_H

#ifdef SHADERTRAP_DEQP
// NOLINTNEXTLINE(build/include_subdir)
#include "glw.h"
#else
#define GL_GLES_PROTOTYPES 0
#include <GLES3/gl32.h>
#endif

#include <functional>

namespace shadertrap {

struct GlFunctions {
  // clang-format off
  // We use camel case for fields so that the GL functions look familiar. We
  // use trailing underscores to avoid these names being redefined when
  // GL-related header files are included.
  std::function<void(GLuint, GLuint)> glActiveShaderProgram_;
  std::function<void(GLenum)> glActiveTexture_;
  std::function<void(GLuint, GLuint)> glAttachShader_;
  std::function<void(GLenum, GLuint)> glBeginQuery_;
  std::function<void(GLenum)> glBeginTransformFeedback_;
  std::function<void(GLuint, GLuint, const GLchar*)> glBindAttribLocation_;
  std::function<void(GLenum, GLuint)> glBindBuffer_;
  std::function<void(GLenum, GLuint, GLuint)> glBindBufferBase_;
  std::function<void(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr)> glBindBufferRange_;
  std::function<void(GLenum, GLuint)> glBindFramebuffer_;
  std::function<void(GLuint, GLuint, GLint, GLboolean, GLint, GLenum, GLenum)> glBindImageTexture_;
  std::function<void(GLuint)> glBindProgramPipeline_;
  std::function<void(GLenum, GLuint)> glBindRenderbuffer_;
  std::function<void(GLuint, GLuint)> glBindSampler_;
  std::function<void(GLenum, GLuint)> glBindTexture_;
  std::function<void(GLenum, GLuint)> glBindTransformFeedback_;
  std::function<void(GLuint)> glBindVertexArray_;
  std::function<void(GLuint, GLuint, GLintptr, GLsizei)> glBindVertexBuffer_;
  std::function<void()> glBlendBarrier_;
  std::function<void(GLfloat, GLfloat, GLfloat, GLfloat)> glBlendColor_;
  std::function<void(GLenum)> glBlendEquation_;
  std::function<void(GLenum, GLenum)> glBlendEquationSeparate_;
  std::function<void(GLuint, GLenum, GLenum)> glBlendEquationSeparatei_;
  std::function<void(GLuint, GLenum)> glBlendEquationi_;
  std::function<void(GLenum, GLenum)> glBlendFunc_;
  std::function<void(GLenum, GLenum, GLenum, GLenum)> glBlendFuncSeparate_;
  std::function<void(GLuint, GLenum, GLenum, GLenum, GLenum)> glBlendFuncSeparatei_;
  std::function<void(GLuint, GLenum, GLenum)> glBlendFunci_;
  std::function<void(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum)> glBlitFramebuffer_;
  std::function<void(GLenum, GLsizeiptr, const void*, GLenum)> glBufferData_;
  std::function<void(GLenum, GLintptr, GLsizeiptr, const void*)> glBufferSubData_;
  std::function<GLenum(GLenum)> glCheckFramebufferStatus_;
  std::function<void(GLbitfield)> glClear_;
  std::function<void(GLenum, GLint, GLfloat, GLint)> glClearBufferfi_;
  std::function<void(GLenum, GLint, const GLfloat*)> glClearBufferfv_;
  std::function<void(GLenum, GLint, const GLint*)> glClearBufferiv_;
  std::function<void(GLenum, GLint, const GLuint*)> glClearBufferuiv_;
  std::function<void(GLfloat, GLfloat, GLfloat, GLfloat)> glClearColor_;
  std::function<void(GLfloat)> glClearDepthf_;
  std::function<void(GLint)> glClearStencil_;
  std::function<GLenum(GLsync, GLbitfield, GLuint64)> glClientWaitSync_;
  std::function<void(GLboolean, GLboolean, GLboolean, GLboolean)> glColorMask_;
  std::function<void(GLuint, GLboolean, GLboolean, GLboolean, GLboolean)> glColorMaski_;
  std::function<void(GLuint)> glCompileShader_;
  std::function<void(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const void*)> glCompressedTexImage2D_;
  std::function<void(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const void*)> glCompressedTexImage3D_;
  std::function<void(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const void*)> glCompressedTexSubImage2D_;
  std::function<void(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const void*)> glCompressedTexSubImage3D_;
  std::function<void(GLenum, GLenum, GLintptr, GLintptr, GLsizeiptr)> glCopyBufferSubData_;
  std::function<void(GLuint, GLenum, GLint, GLint, GLint, GLint, GLuint, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei)> glCopyImageSubData_;
  std::function<void(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint)> glCopyTexImage2D_;
  std::function<void(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)> glCopyTexSubImage2D_;
  std::function<void(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)> glCopyTexSubImage3D_;
  std::function<GLuint()> glCreateProgram_;
  std::function<GLuint(GLenum)> glCreateShader_;
  std::function<GLuint(GLenum, GLsizei, const GLchar*const*)> glCreateShaderProgramv_;
  std::function<void(GLenum)> glCullFace_;
  std::function<void(GLDEBUGPROC, const void*)> glDebugMessageCallback_;
  std::function<void(GLenum, GLenum, GLenum, GLsizei, const GLuint*, GLboolean)> glDebugMessageControl_;
  std::function<void(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*)> glDebugMessageInsert_;
  std::function<void(GLsizei, const GLuint*)> glDeleteBuffers_;
  std::function<void(GLsizei, const GLuint*)> glDeleteFramebuffers_;
  std::function<void(GLuint)> glDeleteProgram_;
  std::function<void(GLsizei, const GLuint*)> glDeleteProgramPipelines_;
  std::function<void(GLsizei, const GLuint*)> glDeleteQueries_;
  std::function<void(GLsizei, const GLuint*)> glDeleteRenderbuffers_;
  std::function<void(GLsizei, const GLuint*)> glDeleteSamplers_;
  std::function<void(GLuint)> glDeleteShader_;
  std::function<void(GLsync)> glDeleteSync_;
  std::function<void(GLsizei, const GLuint*)> glDeleteTextures_;
  std::function<void(GLsizei, const GLuint*)> glDeleteTransformFeedbacks_;
  std::function<void(GLsizei, const GLuint*)> glDeleteVertexArrays_;
  std::function<void(GLenum)> glDepthFunc_;
  std::function<void(GLboolean)> glDepthMask_;
  std::function<void(GLfloat, GLfloat)> glDepthRangef_;
  std::function<void(GLuint, GLuint)> glDetachShader_;
  std::function<void(GLenum)> glDisable_;
  std::function<void(GLuint)> glDisableVertexAttribArray_;
  std::function<void(GLenum, GLuint)> glDisablei_;
  std::function<void(GLuint, GLuint, GLuint)> glDispatchCompute_;
  std::function<void(GLintptr)> glDispatchComputeIndirect_;
  std::function<void(GLenum, GLint, GLsizei)> glDrawArrays_;
  std::function<void(GLenum, const void*)> glDrawArraysIndirect_;
  std::function<void(GLenum, GLint, GLsizei, GLsizei)> glDrawArraysInstanced_;
  std::function<void(GLsizei, const GLenum*)> glDrawBuffers_;
  std::function<void(GLenum, GLsizei, GLenum, const void*)> glDrawElements_;
  std::function<void(GLenum, GLsizei, GLenum, const void*, GLint)> glDrawElementsBaseVertex_;
  std::function<void(GLenum, GLenum, const void*)> glDrawElementsIndirect_;
  std::function<void(GLenum, GLsizei, GLenum, const void*, GLsizei)> glDrawElementsInstanced_;
  std::function<void(GLenum, GLsizei, GLenum, const void*, GLsizei, GLint)> glDrawElementsInstancedBaseVertex_;
  std::function<void(GLenum, GLuint, GLuint, GLsizei, GLenum, const void*)> glDrawRangeElements_;
  std::function<void(GLenum, GLuint, GLuint, GLsizei, GLenum, const void*, GLint)> glDrawRangeElementsBaseVertex_;
  std::function<void(GLenum)> glEnable_;
  std::function<void(GLuint)> glEnableVertexAttribArray_;
  std::function<void(GLenum, GLuint)> glEnablei_;
  std::function<void(GLenum)> glEndQuery_;
  std::function<void()> glEndTransformFeedback_;
  std::function<GLsync(GLenum, GLbitfield)> glFenceSync_;
  std::function<void()> glFinish_;
  std::function<void()> glFlush_;
  std::function<void(GLenum, GLintptr, GLsizeiptr)> glFlushMappedBufferRange_;
  std::function<void(GLenum, GLenum, GLint)> glFramebufferParameteri_;
  std::function<void(GLenum, GLenum, GLenum, GLuint)> glFramebufferRenderbuffer_;
  std::function<void(GLenum, GLenum, GLuint, GLint)> glFramebufferTexture_;
  std::function<void(GLenum, GLenum, GLenum, GLuint, GLint)> glFramebufferTexture2D_;
  std::function<void(GLenum, GLenum, GLuint, GLint, GLint)> glFramebufferTextureLayer_;
  std::function<void(GLenum)> glFrontFace_;
  std::function<void(GLsizei, GLuint*)> glGenBuffers_;
  std::function<void(GLsizei, GLuint*)> glGenFramebuffers_;
  std::function<void(GLsizei, GLuint*)> glGenProgramPipelines_;
  std::function<void(GLsizei, GLuint*)> glGenQueries_;
  std::function<void(GLsizei, GLuint*)> glGenRenderbuffers_;
  std::function<void(GLsizei, GLuint*)> glGenSamplers_;
  std::function<void(GLsizei, GLuint*)> glGenTextures_;
  std::function<void(GLsizei, GLuint*)> glGenTransformFeedbacks_;
  std::function<void(GLsizei, GLuint*)> glGenVertexArrays_;
  std::function<void(GLenum)> glGenerateMipmap_;
  std::function<void(GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*)> glGetActiveAttrib_;
  std::function<void(GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*)> glGetActiveUniform_;
  std::function<void(GLuint, GLuint, GLsizei, GLsizei*, GLchar*)> glGetActiveUniformBlockName_;
  std::function<void(GLuint, GLuint, GLenum, GLint*)> glGetActiveUniformBlockiv_;
  std::function<void(GLuint, GLsizei, const GLuint*, GLenum, GLint*)> glGetActiveUniformsiv_;
  std::function<void(GLuint, GLsizei, GLsizei*, GLuint*)> glGetAttachedShaders_;
  std::function<GLint(GLuint, const GLchar*)> glGetAttribLocation_;
  std::function<void(GLenum, GLuint, GLboolean*)> glGetBooleani_v_;
  std::function<void(GLenum, GLboolean*)> glGetBooleanv_;
  std::function<void(GLenum, GLenum, GLint64*)> glGetBufferParameteri64v_;
  std::function<void(GLenum, GLenum, GLint*)> glGetBufferParameteriv_;
  std::function<void(GLenum, GLenum, void**)> glGetBufferPointerv_;
  std::function<GLuint(GLuint, GLsizei, GLenum*, GLenum*, GLuint*, GLenum*, GLsizei*, GLchar*)> glGetDebugMessageLog_;
  std::function<GLenum()> glGetError_;
  std::function<void(GLenum, GLfloat*)> glGetFloatv_;
  std::function<GLint(GLuint, const GLchar*)> glGetFragDataLocation_;
  std::function<void(GLenum, GLenum, GLenum, GLint*)> glGetFramebufferAttachmentParameteriv_;
  std::function<void(GLenum, GLenum, GLint*)> glGetFramebufferParameteriv_;
  std::function<GLenum()> glGetGraphicsResetStatus_;
  std::function<void(GLenum, GLuint, GLint64*)> glGetInteger64i_v_;
  std::function<void(GLenum, GLint64*)> glGetInteger64v_;
  std::function<void(GLenum, GLuint, GLint*)> glGetIntegeri_v_;
  std::function<void(GLenum, GLint*)> glGetIntegerv_;
  std::function<void(GLenum, GLenum, GLenum, GLsizei, GLint*)> glGetInternalformativ_;
  std::function<void(GLenum, GLuint, GLfloat*)> glGetMultisamplefv_;
  std::function<void(GLenum, GLuint, GLsizei, GLsizei*, GLchar*)> glGetObjectLabel_;
  std::function<void(const void*, GLsizei, GLsizei*, GLchar*)> glGetObjectPtrLabel_;
  std::function<void(GLenum, void**)> glGetPointerv_;
  std::function<void(GLuint, GLsizei, GLsizei*, GLenum*, void*)> glGetProgramBinary_;
  std::function<void(GLuint, GLsizei, GLsizei*, GLchar*)> glGetProgramInfoLog_;
  std::function<void(GLuint, GLenum, GLenum, GLint*)> glGetProgramInterfaceiv_;
  std::function<void(GLuint, GLsizei, GLsizei*, GLchar*)> glGetProgramPipelineInfoLog_;
  std::function<void(GLuint, GLenum, GLint*)> glGetProgramPipelineiv_;
  std::function<GLuint(GLuint, GLenum, const GLchar*)> glGetProgramResourceIndex_;
  std::function<GLint(GLuint, GLenum, const GLchar*)> glGetProgramResourceLocation_;
  std::function<void(GLuint, GLenum, GLuint, GLsizei, GLsizei*, GLchar*)> glGetProgramResourceName_;
  std::function<void(GLuint, GLenum, GLuint, GLsizei, const GLenum*, GLsizei, GLsizei*, GLint*)> glGetProgramResourceiv_;
  std::function<void(GLuint, GLenum, GLint*)> glGetProgramiv_;
  std::function<void(GLuint, GLenum, GLuint*)> glGetQueryObjectuiv_;
  std::function<void(GLenum, GLenum, GLint*)> glGetQueryiv_;
  std::function<void(GLenum, GLenum, GLint*)> glGetRenderbufferParameteriv_;
  std::function<void(GLuint, GLenum, GLint*)> glGetSamplerParameterIiv_;
  std::function<void(GLuint, GLenum, GLuint*)> glGetSamplerParameterIuiv_;
  std::function<void(GLuint, GLenum, GLfloat*)> glGetSamplerParameterfv_;
  std::function<void(GLuint, GLenum, GLint*)> glGetSamplerParameteriv_;
  std::function<void(GLuint, GLsizei, GLsizei*, GLchar*)> glGetShaderInfoLog_;
  std::function<void(GLenum, GLenum, GLint*, GLint*)> glGetShaderPrecisionFormat_;
  std::function<void(GLuint, GLsizei, GLsizei*, GLchar*)> glGetShaderSource_;
  std::function<void(GLuint, GLenum, GLint*)> glGetShaderiv_;
  std::function<const GLubyte*(GLenum)> glGetString_;
  std::function<const GLubyte*(GLenum, GLuint)> glGetStringi_;
  std::function<void(GLsync, GLenum, GLsizei, GLsizei*, GLint*)> glGetSynciv_;
  std::function<void(GLenum, GLint, GLenum, GLfloat*)> glGetTexLevelParameterfv_;
  std::function<void(GLenum, GLint, GLenum, GLint*)> glGetTexLevelParameteriv_;
  std::function<void(GLenum, GLenum, GLint*)> glGetTexParameterIiv_;
  std::function<void(GLenum, GLenum, GLuint*)> glGetTexParameterIuiv_;
  std::function<void(GLenum, GLenum, GLfloat*)> glGetTexParameterfv_;
  std::function<void(GLenum, GLenum, GLint*)> glGetTexParameteriv_;
  std::function<void(GLuint, GLuint, GLsizei, GLsizei*, GLsizei*, GLenum*, GLchar*)> glGetTransformFeedbackVarying_;
  std::function<GLuint(GLuint, const GLchar*)> glGetUniformBlockIndex_;
  std::function<void(GLuint, GLsizei, const GLchar*const*, GLuint*)> glGetUniformIndices_;
  std::function<GLint(GLuint, const GLchar*)> glGetUniformLocation_;
  std::function<void(GLuint, GLint, GLfloat*)> glGetUniformfv_;
  std::function<void(GLuint, GLint, GLint*)> glGetUniformiv_;
  std::function<void(GLuint, GLint, GLuint*)> glGetUniformuiv_;
  std::function<void(GLuint, GLenum, GLint*)> glGetVertexAttribIiv_;
  std::function<void(GLuint, GLenum, GLuint*)> glGetVertexAttribIuiv_;
  std::function<void(GLuint, GLenum, void**)> glGetVertexAttribPointerv_;
  std::function<void(GLuint, GLenum, GLfloat*)> glGetVertexAttribfv_;
  std::function<void(GLuint, GLenum, GLint*)> glGetVertexAttribiv_;
  std::function<void(GLuint, GLint, GLsizei, GLfloat*)> glGetnUniformfv_;
  std::function<void(GLuint, GLint, GLsizei, GLint*)> glGetnUniformiv_;
  std::function<void(GLuint, GLint, GLsizei, GLuint*)> glGetnUniformuiv_;
  std::function<void(GLenum, GLenum)> glHint_;
  std::function<void(GLenum, GLsizei, const GLenum*)> glInvalidateFramebuffer_;
  std::function<void(GLenum, GLsizei, const GLenum*, GLint, GLint, GLsizei, GLsizei)> glInvalidateSubFramebuffer_;
  std::function<GLboolean(GLuint)> glIsBuffer_;
  std::function<GLboolean(GLenum)> glIsEnabled_;
  std::function<GLboolean(GLenum, GLuint)> glIsEnabledi_;
  std::function<GLboolean(GLuint)> glIsFramebuffer_;
  std::function<GLboolean(GLuint)> glIsProgram_;
  std::function<GLboolean(GLuint)> glIsProgramPipeline_;
  std::function<GLboolean(GLuint)> glIsQuery_;
  std::function<GLboolean(GLuint)> glIsRenderbuffer_;
  std::function<GLboolean(GLuint)> glIsSampler_;
  std::function<GLboolean(GLuint)> glIsShader_;
  std::function<GLboolean(GLsync)> glIsSync_;
  std::function<GLboolean(GLuint)> glIsTexture_;
  std::function<GLboolean(GLuint)> glIsTransformFeedback_;
  std::function<GLboolean(GLuint)> glIsVertexArray_;
  std::function<void(GLfloat)> glLineWidth_;
  std::function<void(GLuint)> glLinkProgram_;
  std::function<void*(GLenum, GLintptr, GLsizeiptr, GLbitfield)> glMapBufferRange_;
  std::function<void(GLbitfield)> glMemoryBarrier_;
  std::function<void(GLbitfield)> glMemoryBarrierByRegion_;
  std::function<void(GLfloat)> glMinSampleShading_;
  std::function<void(GLenum, GLuint, GLsizei, const GLchar*)> glObjectLabel_;
  std::function<void(const void*, GLsizei, const GLchar*)> glObjectPtrLabel_;
  std::function<void(GLenum, GLint)> glPatchParameteri_;
  std::function<void()> glPauseTransformFeedback_;
  std::function<void(GLenum, GLint)> glPixelStorei_;
  std::function<void(GLfloat, GLfloat)> glPolygonOffset_;
  std::function<void()> glPopDebugGroup_;
  std::function<void(GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat)> glPrimitiveBoundingBox_;
  std::function<void(GLuint, GLenum, const void*, GLsizei)> glProgramBinary_;
  std::function<void(GLuint, GLenum, GLint)> glProgramParameteri_;
  std::function<void(GLuint, GLint, GLfloat)> glProgramUniform1f_;
  std::function<void(GLuint, GLint, GLsizei, const GLfloat*)> glProgramUniform1fv_;
  std::function<void(GLuint, GLint, GLint)> glProgramUniform1i_;
  std::function<void(GLuint, GLint, GLsizei, const GLint*)> glProgramUniform1iv_;
  std::function<void(GLuint, GLint, GLuint)> glProgramUniform1ui_;
  std::function<void(GLuint, GLint, GLsizei, const GLuint*)> glProgramUniform1uiv_;
  std::function<void(GLuint, GLint, GLfloat, GLfloat)> glProgramUniform2f_;
  std::function<void(GLuint, GLint, GLsizei, const GLfloat*)> glProgramUniform2fv_;
  std::function<void(GLuint, GLint, GLint, GLint)> glProgramUniform2i_;
  std::function<void(GLuint, GLint, GLsizei, const GLint*)> glProgramUniform2iv_;
  std::function<void(GLuint, GLint, GLuint, GLuint)> glProgramUniform2ui_;
  std::function<void(GLuint, GLint, GLsizei, const GLuint*)> glProgramUniform2uiv_;
  std::function<void(GLuint, GLint, GLfloat, GLfloat, GLfloat)> glProgramUniform3f_;
  std::function<void(GLuint, GLint, GLsizei, const GLfloat*)> glProgramUniform3fv_;
  std::function<void(GLuint, GLint, GLint, GLint, GLint)> glProgramUniform3i_;
  std::function<void(GLuint, GLint, GLsizei, const GLint*)> glProgramUniform3iv_;
  std::function<void(GLuint, GLint, GLuint, GLuint, GLuint)> glProgramUniform3ui_;
  std::function<void(GLuint, GLint, GLsizei, const GLuint*)> glProgramUniform3uiv_;
  std::function<void(GLuint, GLint, GLfloat, GLfloat, GLfloat, GLfloat)> glProgramUniform4f_;
  std::function<void(GLuint, GLint, GLsizei, const GLfloat*)> glProgramUniform4fv_;
  std::function<void(GLuint, GLint, GLint, GLint, GLint, GLint)> glProgramUniform4i_;
  std::function<void(GLuint, GLint, GLsizei, const GLint*)> glProgramUniform4iv_;
  std::function<void(GLuint, GLint, GLuint, GLuint, GLuint, GLuint)> glProgramUniform4ui_;
  std::function<void(GLuint, GLint, GLsizei, const GLuint*)> glProgramUniform4uiv_;
  std::function<void(GLuint, GLint, GLsizei, GLboolean, const GLfloat*)> glProgramUniformMatrix2fv_;
  std::function<void(GLuint, GLint, GLsizei, GLboolean, const GLfloat*)> glProgramUniformMatrix2x3fv_;
  std::function<void(GLuint, GLint, GLsizei, GLboolean, const GLfloat*)> glProgramUniformMatrix2x4fv_;
  std::function<void(GLuint, GLint, GLsizei, GLboolean, const GLfloat*)> glProgramUniformMatrix3fv_;
  std::function<void(GLuint, GLint, GLsizei, GLboolean, const GLfloat*)> glProgramUniformMatrix3x2fv_;
  std::function<void(GLuint, GLint, GLsizei, GLboolean, const GLfloat*)> glProgramUniformMatrix3x4fv_;
  std::function<void(GLuint, GLint, GLsizei, GLboolean, const GLfloat*)> glProgramUniformMatrix4fv_;
  std::function<void(GLuint, GLint, GLsizei, GLboolean, const GLfloat*)> glProgramUniformMatrix4x2fv_;
  std::function<void(GLuint, GLint, GLsizei, GLboolean, const GLfloat*)> glProgramUniformMatrix4x3fv_;
  std::function<void(GLenum, GLuint, GLsizei, const GLchar*)> glPushDebugGroup_;
  std::function<void(GLenum)> glReadBuffer_;
  std::function<void(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*)> glReadPixels_;
  std::function<void(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLsizei, void*)> glReadnPixels_;
  std::function<void()> glReleaseShaderCompiler_;
  std::function<void(GLenum, GLenum, GLsizei, GLsizei)> glRenderbufferStorage_;
  std::function<void(GLenum, GLsizei, GLenum, GLsizei, GLsizei)> glRenderbufferStorageMultisample_;
  std::function<void()> glResumeTransformFeedback_;
  std::function<void(GLfloat, GLboolean)> glSampleCoverage_;
  std::function<void(GLuint, GLbitfield)> glSampleMaski_;
  std::function<void(GLuint, GLenum, const GLint*)> glSamplerParameterIiv_;
  std::function<void(GLuint, GLenum, const GLuint*)> glSamplerParameterIuiv_;
  std::function<void(GLuint, GLenum, GLfloat)> glSamplerParameterf_;
  std::function<void(GLuint, GLenum, const GLfloat*)> glSamplerParameterfv_;
  std::function<void(GLuint, GLenum, GLint)> glSamplerParameteri_;
  std::function<void(GLuint, GLenum, const GLint*)> glSamplerParameteriv_;
  std::function<void(GLint, GLint, GLsizei, GLsizei)> glScissor_;
  std::function<void(GLsizei, const GLuint*, GLenum, const void*, GLsizei)> glShaderBinary_;
  std::function<void(GLuint, GLsizei, const GLchar*const*, const GLint*)> glShaderSource_;
  std::function<void(GLenum, GLint, GLuint)> glStencilFunc_;
  std::function<void(GLenum, GLenum, GLint, GLuint)> glStencilFuncSeparate_;
  std::function<void(GLuint)> glStencilMask_;
  std::function<void(GLenum, GLuint)> glStencilMaskSeparate_;
  std::function<void(GLenum, GLenum, GLenum)> glStencilOp_;
  std::function<void(GLenum, GLenum, GLenum, GLenum)> glStencilOpSeparate_;
  std::function<void(GLenum, GLenum, GLuint)> glTexBuffer_;
  std::function<void(GLenum, GLenum, GLuint, GLintptr, GLsizeiptr)> glTexBufferRange_;
  std::function<void(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)> glTexImage2D_;
  std::function<void(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)> glTexImage3D_;
  std::function<void(GLenum, GLenum, const GLint*)> glTexParameterIiv_;
  std::function<void(GLenum, GLenum, const GLuint*)> glTexParameterIuiv_;
  std::function<void(GLenum, GLenum, GLfloat)> glTexParameterf_;
  std::function<void(GLenum, GLenum, const GLfloat*)> glTexParameterfv_;
  std::function<void(GLenum, GLenum, GLint)> glTexParameteri_;
  std::function<void(GLenum, GLenum, const GLint*)> glTexParameteriv_;
  std::function<void(GLenum, GLsizei, GLenum, GLsizei, GLsizei)> glTexStorage2D_;
  std::function<void(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLboolean)> glTexStorage2DMultisample_;
  std::function<void(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei)> glTexStorage3D_;
  std::function<void(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei, GLboolean)> glTexStorage3DMultisample_;
  std::function<void(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*)> glTexSubImage2D_;
  std::function<void(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void*)> glTexSubImage3D_;
  std::function<void(GLuint, GLsizei, const GLchar*const*, GLenum)> glTransformFeedbackVaryings_;
  std::function<void(GLint, GLfloat)> glUniform1f_;
  std::function<void(GLint, GLsizei, const GLfloat*)> glUniform1fv_;
  std::function<void(GLint, GLint)> glUniform1i_;
  std::function<void(GLint, GLsizei, const GLint*)> glUniform1iv_;
  std::function<void(GLint, GLuint)> glUniform1ui_;
  std::function<void(GLint, GLsizei, const GLuint*)> glUniform1uiv_;
  std::function<void(GLint, GLfloat, GLfloat)> glUniform2f_;
  std::function<void(GLint, GLsizei, const GLfloat*)> glUniform2fv_;
  std::function<void(GLint, GLint, GLint)> glUniform2i_;
  std::function<void(GLint, GLsizei, const GLint*)> glUniform2iv_;
  std::function<void(GLint, GLuint, GLuint)> glUniform2ui_;
  std::function<void(GLint, GLsizei, const GLuint*)> glUniform2uiv_;
  std::function<void(GLint, GLfloat, GLfloat, GLfloat)> glUniform3f_;
  std::function<void(GLint, GLsizei, const GLfloat*)> glUniform3fv_;
  std::function<void(GLint, GLint, GLint, GLint)> glUniform3i_;
  std::function<void(GLint, GLsizei, const GLint*)> glUniform3iv_;
  std::function<void(GLint, GLuint, GLuint, GLuint)> glUniform3ui_;
  std::function<void(GLint, GLsizei, const GLuint*)> glUniform3uiv_;
  std::function<void(GLint, GLfloat, GLfloat, GLfloat, GLfloat)> glUniform4f_;
  std::function<void(GLint, GLsizei, const GLfloat*)> glUniform4fv_;
  std::function<void(GLint, GLint, GLint, GLint, GLint)> glUniform4i_;
  std::function<void(GLint, GLsizei, const GLint*)> glUniform4iv_;
  std::function<void(GLint, GLuint, GLuint, GLuint, GLuint)> glUniform4ui_;
  std::function<void(GLint, GLsizei, const GLuint*)> glUniform4uiv_;
  std::function<void(GLuint, GLuint, GLuint)> glUniformBlockBinding_;
  std::function<void(GLint, GLsizei, GLboolean, const GLfloat*)> glUniformMatrix2fv_;
  std::function<void(GLint, GLsizei, GLboolean, const GLfloat*)> glUniformMatrix2x3fv_;
  std::function<void(GLint, GLsizei, GLboolean, const GLfloat*)> glUniformMatrix2x4fv_;
  std::function<void(GLint, GLsizei, GLboolean, const GLfloat*)> glUniformMatrix3fv_;
  std::function<void(GLint, GLsizei, GLboolean, const GLfloat*)> glUniformMatrix3x2fv_;
  std::function<void(GLint, GLsizei, GLboolean, const GLfloat*)> glUniformMatrix3x4fv_;
  std::function<void(GLint, GLsizei, GLboolean, const GLfloat*)> glUniformMatrix4fv_;
  std::function<void(GLint, GLsizei, GLboolean, const GLfloat*)> glUniformMatrix4x2fv_;
  std::function<void(GLint, GLsizei, GLboolean, const GLfloat*)> glUniformMatrix4x3fv_;
  std::function<GLboolean(GLenum)> glUnmapBuffer_;
  std::function<void(GLuint)> glUseProgram_;
  std::function<void(GLuint, GLbitfield, GLuint)> glUseProgramStages_;
  std::function<void(GLuint)> glValidateProgram_;
  std::function<void(GLuint)> glValidateProgramPipeline_;
  std::function<void(GLuint, GLfloat)> glVertexAttrib1f_;
  std::function<void(GLuint, const GLfloat*)> glVertexAttrib1fv_;
  std::function<void(GLuint, GLfloat, GLfloat)> glVertexAttrib2f_;
  std::function<void(GLuint, const GLfloat*)> glVertexAttrib2fv_;
  std::function<void(GLuint, GLfloat, GLfloat, GLfloat)> glVertexAttrib3f_;
  std::function<void(GLuint, const GLfloat*)> glVertexAttrib3fv_;
  std::function<void(GLuint, GLfloat, GLfloat, GLfloat, GLfloat)> glVertexAttrib4f_;
  std::function<void(GLuint, const GLfloat*)> glVertexAttrib4fv_;
  std::function<void(GLuint, GLuint)> glVertexAttribBinding_;
  std::function<void(GLuint, GLuint)> glVertexAttribDivisor_;
  std::function<void(GLuint, GLint, GLenum, GLboolean, GLuint)> glVertexAttribFormat_;
  std::function<void(GLuint, GLint, GLint, GLint, GLint)> glVertexAttribI4i_;
  std::function<void(GLuint, const GLint*)> glVertexAttribI4iv_;
  std::function<void(GLuint, GLuint, GLuint, GLuint, GLuint)> glVertexAttribI4ui_;
  std::function<void(GLuint, const GLuint*)> glVertexAttribI4uiv_;
  std::function<void(GLuint, GLint, GLenum, GLuint)> glVertexAttribIFormat_;
  std::function<void(GLuint, GLint, GLenum, GLsizei, const void*)> glVertexAttribIPointer_;
  std::function<void(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)> glVertexAttribPointer_;
  std::function<void(GLuint, GLuint)> glVertexBindingDivisor_;
  std::function<void(GLint, GLint, GLsizei, GLsizei)> glViewport_;
  std::function<void(GLsync, GLbitfield, GLuint64)> glWaitSync_;
  // clang-format on
};

}  // namespace shadertrap

#endif  // LIBSHADERTRAP_GL_FUNCTIONS_H
