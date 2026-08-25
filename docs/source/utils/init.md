# `source/utils/init.h` — Design Architecture & Notes

Explanatory and architectural design notes extracted from source code and replaced with concise technical Doxygen blocks. This document preserves the reasoning ('why') separated from technical API documentation.

## `SOLOADER_INIT_H` (line ~1)

**Source File:** `source/utils/init.h`

> Copyright (C) 2022-2024 Volodymyr Atamanenko
>
> This software may be modified and distributed under the terms
> of the MIT license. See the LICENSE file for details.

---

## `patch_get_and_reset_blit_calls` (line ~30)

**Source File:** `source/utils/init.h`

> Bug 16 (PORTING_PLAN.md), 4th perf pass: real hardware GL instrumentation
> showed draws=1/binds=1 per frame (a single full-screen quad), meaning the
> .so composites the scene by software into a 480x320 buffer before ever
> touching GL (uploaded whole via glTexSubImage2D every frame -- see
> glutil.c). PutCompressImg (.so+0x140050, decompiled in
> out_ghidra.c:258914) is the common sprite-blit dispatcher this .so
> exports -- same function name/role Zenonia 4 (same GxPZx engine family)
> used for its own proven hot-path probe. This counts calls/frame to it --
> a non-destructive hook (runs the untouched original every time) to
> confirm it's the real hot path and that it scales with enemy count.

---

## `patch_format_and_reset_blit_histogram` (line ~41)

**Source File:** `source/utils/init.h`

> Writes "OPNAME:count,OPNAME:count,..." (only non-zero enumDrawOP buckets
> for this frame) into buf and resets the per-op histogram. See patch.c for
> the enumDrawOP -> name mapping (from CMvGraphics::InitialBlend()).

---
