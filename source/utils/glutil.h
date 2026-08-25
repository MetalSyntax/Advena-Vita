/**
 * @brief Copyright (C) 2021 Andy Nguyen Copyright (C) 2021 Rinnegatamante Copyright (C) 2022-2023 Volodymyr Atamanenko This software may be modified.
 * @note See `docs/source/utils/glutil.md:1` for detailed design rationale.
 */

/**
 * @brief Copyright (C) 2021 Andy Nguyen Copyright (C) 2021 Rinnegatamante Copyright (C) 2022-2023 Volodymyr Atamanenko This software may be modified.
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
/**
 * @brief Bug 16 (PORTING_PLAN.md) diagnostic instrumentation.
 * @note See `docs/source/utils/glutil.md:36` for detailed design rationale.
 */
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
