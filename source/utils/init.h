/*
 * Copyright (C) 2022-2024 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

/**
 * @file  init.h
 * @brief so-loader initialization routines.
 */

#ifndef SOLOADER_INIT_H
#define SOLOADER_INIT_H

#include <so_util/so_util.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void resolve_imports(so_module *mod);

void so_patch();

void soloader_init_all();

#ifdef INSTRUMENT_BLIT_CALLS
// Bug 16 (PORTING_PLAN.md), 4th perf pass: real hardware GL instrumentation
// showed draws=1/binds=1 per frame (a single full-screen quad), meaning the
// .so composites the scene by software into a 480x320 buffer before ever
// touching GL (uploaded whole via glTexSubImage2D every frame -- see
// glutil.c). PutCompressImg (.so+0x140050, decompiled in
// out_ghidra.c:258914) is the common sprite-blit dispatcher this .so
// exports -- same function name/role Zenonia 4 (same GxPZx engine family)
// used for its own proven hot-path probe. This counts calls/frame to it --
// a non-destructive hook (runs the untouched original every time) to
// confirm it's the real hot path and that it scales with enemy count.
uint32_t patch_get_and_reset_blit_calls();
// Writes "OPNAME:count,OPNAME:count,..." (only non-zero enumDrawOP buckets
// for this frame) into buf and resets the per-op histogram. See patch.c for
// the enumDrawOP -> name mapping (from CMvGraphics::InitialBlend()).
void patch_format_and_reset_blit_histogram(char *buf, int buflen);
#endif

#ifdef __cplusplus
};
#endif

#endif // SOLOADER_INIT_H
