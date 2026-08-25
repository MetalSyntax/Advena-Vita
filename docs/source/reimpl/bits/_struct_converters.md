# `source/reimpl/bits/_struct_converters.c` — Design Architecture & Notes

Explanatory and architectural design notes extracted from source code and replaced with concise technical Doxygen blocks. This document preserves the reasoning ('why') separated from technical API documentation.

## `SC_INLINE` (line ~1)

**Source File:** `source/reimpl/bits/_struct_converters.c`

> Copyright (C) 2022-2024 Volodymyr Atamanenko
>
> This software may be modified and distributed under the terms
> of the MIT license. See the LICENSE file for details.

---

## `out` (line ~29)

**Source File:** `source/reimpl/bits/_struct_converters.c`

> Convert bionic (Android) `open()` flags to newlib (Vita) flags
>
> @param[in] flags open() flags created using musl defines
>
> @return open(flags) recreated using newlib defines

---

## `_struct_converters.c` (line ~57) (line ~57)

**Source File:** `source/reimpl/bits/_struct_converters.c`

> Convert newlib (Vita) `dirent` struct to bionic (Android) format.
>
> @param[in] dirent_newlib Pointer to a newlib-format dirent struct
>
> @return Pointer to a bionic-format dirent struct.
>         Must be freed by the caller.

---

## `stat_newlib_to_bionic` (line ~75)

**Source File:** `source/reimpl/bits/_struct_converters.c`

> Convert newlib (Vita) `stat` struct to bionic (Android) format.
> @param[in]  src Pointer to a newlib-format stat struct
> @param[out] dst Pointer to a bionic-format stat struct

---
