/*
 * Copyright (C) 2021      Andy Nguyen
 * Copyright (C) 2021      Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/glutil.h"

#include "utils/utils.h"
#include "utils/dialog.h"
#include "utils/logger.h"
#include "utils/init.h"

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/io/stat.h>

// Helpers for our handling of shaders
GLboolean skip_next_compile = GL_FALSE;
char next_shader_fname[256];
void load_shader(GLuint shader, const char * string, size_t length);

void gl_preload() {
    if (!file_exists("ur0:/data/libshacccg.suprx")
        && !file_exists("ur0:/data/external/libshacccg.suprx")) {
        fatal_error("Error: libshacccg.suprx is not installed. "
                    "Google \"ShaRKBR33D\" for quick installation.");
    }

#ifdef USE_GLSL_SHADERS
    vglSetSemanticBindingMode(VGL_MODE_POSTPONED);
#endif
}

void gl_init() {
    // No MSAA / no triple buffering: this is the config confirmed working on
    // real hardware for the same engine (com.gamevil.nexus2.Natives) in the
    // Zenonia 2/3 Vita ports and in Prince of Persia. Advena was the only
    // port in this family initializing with SCE_GXM_MULTISAMPLE_4X instead
    // of _NONE. Re-confirmed "sin cambios" during the Bug 16 perf pass
    // (PORTING_PLAN.md) -- left untouched on purpose, do not enable triple
    // buffering here without new hardware evidence for this specific port.
    vglUseTripleBuffering(GL_FALSE);
    // Bug 16 (PORTING_PLAN.md), 3rd perf pass: this port's .so uploads real
    // client-side vertex arrays (glVertexPointer/glTexCoordPointer + real
    // glDrawArrays/glDrawElements) every frame instead of using VBOs -- with
    // 10+ enemies attacking at once that's a lot of small CPU writes into
    // vitaGL's internal pools each frame. vitaGL's default pools are
    // uncached memory (slow CPU writes, fast GPU reads); switching to cached
    // memory makes those per-draw CPU writes fast at the cost of vitaGL
    // having to flush the cache before the GPU reads it, which vitaGL does
    // internally -- this is the standard fix for CPU-write-bound immediate-
    // mode ports like this one. Must be called before vglInit*.
    vglUseCachedMem(GL_TRUE);
    // Legacy immediate-mode pool bumped 6MB -> 8MB: this port issues real
    // glDrawArrays/glDrawElements calls directly from the .so (client-side
    // vertex arrays, not VBOs -- see PORTING_PLAN.md Bug 16), so headroom
    // here avoids vitaGL falling back to on-demand pool growth/reclaim mid-
    // frame during busy combat scenes. Does not touch the MSAA/triple-
    // buffering config above.
    vglInitExtended(0, 960, 544, 8 * 1024 * 1024, SCE_GXM_MULTISAMPLE_NONE);
}

void gl_swap() {
    vglSwapBuffers(GL_FALSE);
}

#ifdef INSTRUMENT_GL_CALLS
// Bug 16 (PORTING_PLAN.md) diagnostic: Advena renders via real GL calls
// (unlike Zenonia 4's software rasterizer), so the equivalent of that port's
// PutCompressImg hot-path probe is counting draw calls and texture bind/
// switch volume per frame. Report via game_log() (unconditional -- l_info()
// compiles to nothing outside DEBUG_SOLOADER builds, which would make this
// build flag silently produce no output).
static uint32_t instr_draw_calls = 0;
static uint32_t instr_bind_calls = 0;
static uint32_t instr_texture_switches = 0;
static GLuint instr_last_texture = (GLuint) -1;
// 4th perf pass (PORTING_PLAN.md Bug 16): draws=1/binds=1 per frame (measured
// on real hardware, whole play session incl. combat) rules out per-sprite GL
// draw calls as the bottleneck -- the .so blits a single full-frame quad per
// frame, meaning it composites by software first (same architecture family
// as Zenonia 4's CGxPZxZero::Blt). This tracks whether that single quad's
// texture is being re-uploaded (glTexImage2D) or updated (glTexSubImage2D)
// every frame, and how many pixels that costs -- the likely real hot path.
static uint32_t instr_teximage_calls = 0;
static uint32_t instr_texsubimage_calls = 0;
static uint64_t instr_teximage_pixels = 0;
static uint64_t instr_texsubimage_pixels = 0;

void glDrawArrays_soloader(GLenum mode, GLint first, GLsizei count) {
    instr_draw_calls++;
    glDrawArrays(mode, first, count);
}

void glDrawElements_soloader(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    instr_draw_calls++;
    glDrawElements(mode, count, type, indices);
}

void glBindTexture_soloader(GLenum target, GLuint texture) {
    instr_bind_calls++;
    if (texture != instr_last_texture) {
        instr_texture_switches++;
        instr_last_texture = texture;
    }
    glBindTexture(target, texture);
}

void glTexImage2D_soloader(GLenum target, GLint level, GLint internalformat,
                            GLsizei width, GLsizei height, GLint border,
                            GLenum format, GLenum type, const void *pixels) {
    instr_teximage_calls++;
    instr_teximage_pixels += (uint64_t) width * (uint64_t) height;
    glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

void glTexSubImage2D_soloader(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                               GLsizei width, GLsizei height, GLenum format,
                               GLenum type, const void *pixels) {
    instr_texsubimage_calls++;
    instr_texsubimage_pixels += (uint64_t) width * (uint64_t) height;
    glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void gl_instrument_frame_end() {
#ifdef INSTRUMENT_BLIT_CALLS
    uint32_t blits = patch_get_and_reset_blit_calls();
    char blit_hist[256];
    patch_format_and_reset_blit_histogram(blit_hist, sizeof(blit_hist));
    game_log("[PERF] GL/frame: draws=%u binds=%u tex_switches=%u teximg=%u teximg_px=%llu texsubimg=%u texsubimg_px=%llu blits=%u ops=%s\n",
             instr_draw_calls, instr_bind_calls, instr_texture_switches,
             instr_teximage_calls, (unsigned long long) instr_teximage_pixels,
             instr_texsubimage_calls, (unsigned long long) instr_texsubimage_pixels,
             blits, blit_hist);
#else
    game_log("[PERF] GL/frame: draws=%u binds=%u tex_switches=%u teximg=%u teximg_px=%llu texsubimg=%u texsubimg_px=%llu\n",
             instr_draw_calls, instr_bind_calls, instr_texture_switches,
             instr_teximage_calls, (unsigned long long) instr_teximage_pixels,
             instr_texsubimage_calls, (unsigned long long) instr_texsubimage_pixels);
#endif
    instr_draw_calls = 0;
    instr_bind_calls = 0;
    instr_texture_switches = 0;
    instr_teximage_calls = 0;
    instr_texsubimage_calls = 0;
    instr_teximage_pixels = 0;
    instr_texsubimage_pixels = 0;
}
#endif

void glShaderSource_soloader(GLuint shader, GLsizei count,
                             const GLchar **string, const GLint *_length) {
#ifdef DEBUG_OPENGL
    sceClibPrintf("[gl_dbg] glShaderSource<%p>(shader: %i, count: %i, string: %p, length: %p)\n", __builtin_return_address(0), shader, count, string, _length);
#endif
    if (!string) {
        l_error("<%p> Shader source string is NULL, count: %i",
                   __builtin_return_address(0), count);
        skip_next_compile = GL_TRUE;
        return;
    } else if (!*string) {
        l_error("<%p> Shader source *string is NULL, count: %i",
                   __builtin_return_address(0), count);
        skip_next_compile = GL_TRUE;
        return;
    }

    size_t total_length = 0;

    for (int i = 0; i < count; ++i) {
        if (!_length) {
            total_length += strlen(string[i]);
        } else {
            total_length += _length[i];
        }
    }

    char * str = malloc(total_length+1);
    size_t l = 0;

    for (int i = 0; i < count; ++i) {
        if (!_length) {
            memcpy(str + l, string[i], strlen(string[i]));
            l += strlen(string[i]);
        } else {
            memcpy(str + l, string[i], _length[i]);
            l += _length[i];
        }
    }
    str[total_length] = '\0';

    load_shader(shader, str, total_length);

    free(str);
}

void glCompileShader_soloader(GLuint shader) {
#ifdef DEBUG_OPENGL
    sceClibPrintf("[gl_dbg] glCompileShader<%p>(shader: %i)\n", __builtin_return_address(0), shader);
#endif

#ifndef USE_GXP_SHADERS
    if (!skip_next_compile) {
        glCompileShader(shader);
#ifdef DUMP_COMPILED_SHADERS
        void *bin = vglMalloc(32 * 1024);
        GLsizei len;
        vglGetShaderBinary(shader, 32 * 1024, &len, bin);
        file_save(next_shader_fname, bin, len);
        vglFree(bin);
#endif
    }
    skip_next_compile = GL_FALSE;
#endif
}

#if defined(USE_GLSL_SHADERS) && defined(DUMP_COMPILED_SHADERS)
void load_shader(GLuint shader, const char * string, size_t length) {
    char* sha_name = str_sha1sum(string, length);

    char gxp_path[256];
    snprintf(gxp_path, sizeof(gxp_path), DATA_PATH"gxp/%s.gxp", sha_name);

    if (file_exists(gxp_path)) {
        uint8_t *buffer;
        size_t size;

        file_load(gxp_path, &buffer, &size);

        glShaderBinary(1, &shader, 0, buffer, (int32_t) size);

        free(buffer);
        skip_next_compile = GL_TRUE;
    } else {
        glShaderSource(shader, 1, &string, &length);
        strcpy(next_shader_fname, gxp_path);
    }

    free(sha_name);
}
#elif defined(USE_GLSL_SHADERS)
void load_shader(GLuint shader, const char * string, size_t length) {
    glShaderSource(shader, 1, &string, &length);
}
#elif defined(USE_CG_SHADERS) && defined(DUMP_COMPILED_SHADERS)
void load_shader(GLuint shader, const char * string, size_t length) {
    char* sha_name = str_sha1sum(string, length);

    char gxp_path[256];
    char cg_path[256];
    snprintf(gxp_path, sizeof(gxp_path), DATA_PATH"gxp/%s.gxp", sha_name);
    snprintf(cg_path, sizeof(cg_path), DATA_PATH"cg/%s.cg", sha_name);

    if (file_exists(gxp_path)) {
        uint8_t *buffer;
        size_t size;

        file_load(gxp_path, &buffer, &size);

        glShaderBinary(1, &shader, 0, buffer, (int32_t) size);

        free(buffer);
        skip_next_compile = GL_TRUE;
    } else if (file_exists(cg_path)) {
        char *buffer;
        size_t size;

        file_load(cg_path, (uint8_t **) &buffer, &size);

        glShaderSource(shader, 1, &string, &size);
        strcpy(next_shader_fname, gxp_path);

        free(buffer);
        skip_next_compile = GL_FALSE;
    } else {
        l_warn("Encountered an untranslated shader %s, saving GLSL "
               "and using a dummy shader.", sha_name);

        char glsl_path[256];
        snprintf(glsl_path, sizeof(glsl_path), DATA_PATH"glsl/%s.glsl", sha_name);
        file_mkpath(glsl_path, 0777);
        file_save(glsl_path, (const uint8_t *) string, length);

        if (strstr(string, "gl_FragColor")) {
            const char *dummy_shader = "float4 main() { return float4(1.0,1.0,1.0,1.0); }";
            int32_t dummy_shader_len = (int32_t) strlen(dummy_shader);
            glShaderSource(shader, 1, &dummy_shader, &dummy_shader_len);
        } else {
            const char *dummy_shader = "void main(float4 out gl_Position : POSITION ) { gl_Position = float4(1.0,1.0,1.0,1.0); }";
            int32_t dummy_shader_len = (int32_t) strlen(dummy_shader);
            glShaderSource(shader, 1, &dummy_shader, &dummy_shader_len);
        }

        skip_next_compile = GL_FALSE;
    }

    free(sha_name);
}
#elif defined(USE_CG_SHADERS) || defined(USE_GXP_SHADERS)
void load_shader(GLuint shader, const char * string, size_t length) {
    char* sha_name = str_sha1sum(string, length);

    char path[256];
#ifdef USE_CG_SHADERS
    snprintf(path, sizeof(path), DATA_PATH"cg/%s.cg", sha_name);
#else
    snprintf(path, sizeof(path), DATA_PATH"gxp/%s.gxp", sha_name);
#endif

    if (file_exists(path)) {
#ifdef USE_CG_SHADERS
        char *buffer;
        size_t size;

        file_load(path, (uint8_t **) &buffer, &size);

        glShaderSource(shader, 1, &string, &size);

        free(buffer);
#else
        uint8_t *buffer;
        size_t size;

        file_load(path, &buffer, &size);

        glShaderBinary(1, &shader, 0, buffer, (int32_t) size);

        free(buffer);
#endif
    } else {
        l_warn("Encountered an untranslated shader %s, saving GLSL "
               "and using a dummy shader.", sha_name);

        char glsl_path[256];
        snprintf(glsl_path, sizeof(glsl_path), DATA_PATH"glsl/%s.glsl", sha_name);
        file_mkpath(glsl_path, 0777);
        file_save(glsl_path, (const uint8_t *) string, length);

        if (strstr(string, "gl_FragColor")) {
            const char *dummy_shader = "float4 main() { return float4(1.0,1.0,1.0,1.0); }";
            int32_t dummy_shader_len = (int32_t) strlen(dummy_shader);
            glShaderSource(shader, 1, &dummy_shader, &dummy_shader_len);
        } else {
            const char *dummy_shader = "void main(float4 out gl_Position : POSITION ) { gl_Position = float4(1.0,1.0,1.0,1.0); }";
            int32_t dummy_shader_len = (int32_t) strlen(dummy_shader);
            glShaderSource(shader, 1, &dummy_shader, &dummy_shader_len);
        }
    }

    free(sha_name);
}
#else
#error "Define one of (USE_GLSL_SHADERS, USE_CG_SHADERS, USE_GXP_SHADERS)"
#endif
