# `source/utils/glutil.h` — Design Architecture & Notes

Explanatory and architectural design notes extracted from source code and replaced with concise technical Doxygen blocks. This document preserves the reasoning ('why') separated from technical API documentation.

## `SOLOADER_GLUTIL_H` (line ~1)

**Source File:** `source/utils/glutil.h`

> Copyright (C) 2021      Andy Nguyen
> Copyright (C) 2021      Rinnegatamante
> Copyright (C) 2022-2023 Volodymyr Atamanenko
>
> This software may be modified and distributed under the terms
> of the MIT license. See the LICENSE file for details.

---

## `glDrawArrays_soloader` (line ~36)

**Source File:** `source/utils/glutil.h`

> Bug 16 (PORTING_PLAN.md) diagnostic instrumentation: count draw calls and
> texture binds/switches per frame, the GL-pipeline equivalent of Zenonia
> 4's PutCompressImg hot-path probe. Zero-cost when INSTRUMENT_GL_CALLS is
> not defined (dynlib.c resolves the .so's imports straight to the real
> vitaGL functions instead of these wrappers).

---
