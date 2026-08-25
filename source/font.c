/**
 * @brief font.c — stb_truetype rasterizer for Gamevil Font API (GFA).
 * @note See `docs/source/font.md:1` for detailed design rationale.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb/stb_truetype.h>

#include "font.h"
#include "utils/logger.h"

static unsigned char *font_blob = NULL;
static stbtt_fontinfo font_info;
static int font_ok = 0;

int gfa_font_init(const char *path) {
    if (font_ok) return 1;

    FILE *f = fopen(path, "rb");
    if (!f) {
        l_warn("[FONT] Could not open %s", path);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    font_blob = (unsigned char *) malloc(size);
    if (!font_blob) { fclose(f); return 0; }
    fread(font_blob, 1, size, f);
    fclose(f);

    if (!stbtt_InitFont(&font_info, font_blob,
                        stbtt_GetFontOffsetForIndex(font_blob, 0))) {
        l_warn("[FONT] stbtt_InitFont failed for %s", path);
        free(font_blob);
        font_blob = NULL;
        return 0;
    }
    font_ok = 1;
    l_info("[FONT] %s loaded (%ld bytes)", path, size);
    return 1;
}

int gfa_font_ready(void) {
    return font_ok;
}

static float px_scale(float px) {
    return stbtt_ScaleForMappingEmToPixels(&font_info, px);
}

int gfa_font_ascent(float px) {
    if (!font_ok) return (int) ceilf(px * 0.8f);
    int a, d, lg;
    stbtt_GetFontVMetrics(&font_info, &a, &d, &lg);
    return (int) ceilf((float) a * px_scale(px));
}

int gfa_font_descent(float px) {
    if (!font_ok) return (int) ceilf(px * 0.2f);
    int a, d, lg;
    stbtt_GetFontVMetrics(&font_info, &a, &d, &lg);
    return (int) ceilf((float) (-d) * px_scale(px));
}

float gfa_font_advance(float px, uint32_t cp) {
    if (!font_ok) return px * 0.5f;
    int glyph = stbtt_FindGlyphIndex(&font_info, (int) cp);
    if (!glyph) return px * 0.5f;
    int adv, lsb;
    stbtt_GetGlyphHMetrics(&font_info, glyph, &adv, &lsb);
    return (float) adv * px_scale(px);
}

float gfa_font_text_width(float px, const uint32_t *cps, int n) {
    float w = 0.0f;
    for (int i = 0; i < n; i++) {
        w += gfa_font_advance(px, cps[i]);
    }
    return w;
}

int gfa_font_break_text(float px, const uint32_t *cps, int n, float max_width) {
    float w = 0.0f;
    for (int i = 0; i < n; i++) {
        float adv = gfa_font_advance(px, cps[i]);
        if (w + adv > max_width && i > 0) return i;
        w += adv;
    }
    return n;
}

float gfa_font_draw_line(float px, const uint32_t *cps, int n,
                         uint32_t *buf, int bw, int bh,
                         float pen_x, float baseline_y, uint32_t color) {
    if (!font_ok || !buf) return 0.0f;

    float scale = px_scale(px);
    float cur_x = pen_x;

    uint32_t color_rgb = color & 0x00ffffff;
    float color_a = (float) ((color >> 24) & 0xff) / 255.0f;

    for (int i = 0; i < n; i++) {
        uint32_t cp = cps[i];
        int glyph = stbtt_FindGlyphIndex(&font_info, (int) cp);
        int adv, lsb;
        stbtt_GetGlyphHMetrics(&font_info, glyph, &adv, &lsb);

        if (glyph != 0) {
            int ix0, iy0, ix1, iy1;
            stbtt_GetGlyphBitmapBoxSubpixel(&font_info, glyph, scale, scale,
                                            cur_x - floorf(cur_x), 0.0f,
                                            &ix0, &iy0, &ix1, &iy1);
            int gw = ix1 - ix0;
            int gh = iy1 - iy0;
            if (gw > 0 && gh > 0) {
                unsigned char *bmp = (unsigned char *) malloc(gw * gh);
                if (bmp) {
                    stbtt_MakeGlyphBitmapSubpixel(&font_info, bmp, gw, gh, gw,
                                                  scale, scale,
                                                  cur_x - floorf(cur_x), 0.0f, glyph);
                    int dst_x0 = (int) floorf(cur_x) + ix0;
                    int dst_y0 = (int) floorf(baseline_y) + iy0;

                    for (int gy = 0; gy < gh; gy++) {
                        int dy = dst_y0 + gy;
                        if (dy < 0 || dy >= bh) continue;
                        for (int gx = 0; gx < gw; gx++) {
                            int dx = dst_x0 + gx;
                            if (dx < 0 || dx >= bw) continue;
                            unsigned char cov = bmp[gy * gw + gx];
                            if (cov == 0) continue;

                            int final_a = (int) ((float) cov * color_a);
                            if (final_a > 255) final_a = 255;
                            buf[dy * bw + dx] = color_rgb | ((uint32_t) final_a << 24);
                        }
                    }
                    free(bmp);
                }
            }
        }
        cur_x += (float) adv * scale;
    }
    return cur_x - pen_x;
}
