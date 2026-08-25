# `source/dynlib.c` — Design Architecture & Notes

Explanatory and architectural design notes extracted from source code and replaced with concise technical Doxygen blocks. This document preserves the reasoning ('why') separated from technical API documentation.

## `dynlib.c` (line ~1) (line ~1)

**Source File:** `source/dynlib.c`

> Copyright (C) 2021      Andy Nguyen
> Copyright (C) 2021      Rinnegatamante
> Copyright (C) 2022-2024 Volodymyr Atamanenko
>
> This software may be modified and distributed under the terms
> of the MIT license. See the LICENSE file for details.

---

## `GL_DRAW_ARRAYS_IMPL` (line ~46)

**Source File:** `source/dynlib.c`

> Bug 16 (PORTING_PLAN.md) diagnostic instrumentation: resolve these 3
> imports to counting wrappers instead of the real vitaGL functions when
> enabled. See glutil.c for the counters/report.

---

## `dynlib.c` (line ~144) (line ~144)

**Source File:** `source/dynlib.c`

> Usage example:
> if (strcmp("AMotionEvent_getAxisValue", symbol) == 0)
> return &AMotionEvent_getAxisValue;

---
