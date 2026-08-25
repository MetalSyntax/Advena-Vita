#ifndef __FONT_H__
#define __FONT_H__

#include <stdint.h>

/**
 * @brief Font raster backend for GFA bridge (NexusFont.java).
 * @note See `docs/source/font.md:6` for detailed design rationale.
 */

int gfa_font_init(const char *path); // 1 si cargo bien (idempotente)
int gfa_font_ready(void);

int gfa_font_ascent(float px);
int gfa_font_descent(float px);

/**
 * @brief Error 500 (Server Error).
 * @note See `docs/source/font.md:25` for detailed design rationale.
 */
float gfa_font_advance(float px, uint32_t cp);
float gfa_font_text_width(float px, const uint32_t *cps, int n);

/**
 * @brief Paint.breakText(text, true, maxWidth, null).
 * @note See `docs/source/font.md:29` for detailed design rationale.
 */
int gfa_font_break_text(float px, const uint32_t *cps, int n, float max_width);

/**
 * @brief Draw a line of text in buf (bw*bh ARGB pixels), initial pen in pen_x, baseline in baseline_y.
 * @note See `docs/source/font.md:33` for detailed design rationale.
 */
float gfa_font_draw_line(float px, const uint32_t *cps, int n, uint32_t *buf,
                         int bw, int bh, float pen_x, float baseline_y,
                         uint32_t color);

#endif
