/*
 * Copyright (C) 2021      Andy Nguyen
 * Copyright (C) 2021      Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

/**
 * @file  glutil.h
 * @brief OpenGL API initializer, related functions.
 */

#ifndef SOLOADER_GLUTIL_H
#define SOLOADER_GLUTIL_H

#include <vitaGL.h>

#ifdef __cplusplus
extern "C" {
#endif

void gl_init();

void gl_preload();

void gl_swap();

void glCompileShader_soloader(GLuint shader);

void glShaderSource_soloader(GLuint shader, GLsizei count,
                             const GLchar **string, const GLint *_length);

#ifdef INSTRUMENT_GL_CALLS
// Bug 16 (PORTING_PLAN.md) diagnostic instrumentation: count draw calls and
// texture binds/switches per frame, the GL-pipeline equivalent of Zenonia
// 4's PutCompressImg hot-path probe. Zero-cost when INSTRUMENT_GL_CALLS is
// not defined (dynlib.c resolves the .so's imports straight to the real
// vitaGL functions instead of these wrappers).
void glDrawArrays_soloader(GLenum mode, GLint first, GLsizei count);
void glDrawElements_soloader(GLenum mode, GLsizei count, GLenum type, const void *indices);
void glBindTexture_soloader(GLenum target, GLuint texture);
void glTexImage2D_soloader(GLenum target, GLint level, GLint internalformat,
                            GLsizei width, GLsizei height, GLint border,
                            GLenum format, GLenum type, const void *pixels);
void glTexSubImage2D_soloader(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                               GLsizei width, GLsizei height, GLenum format,
                               GLenum type, const void *pixels);
void gl_instrument_frame_end();
#endif

#ifdef __cplusplus
};
#endif

#endif // SOLOADER_GLUTIL_H
