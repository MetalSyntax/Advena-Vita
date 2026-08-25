# `source/utils/utils.h` — Design Architecture & Notes

Explanatory and architectural design notes extracted from source code and replaced with concise technical Doxygen blocks. This document preserves the reasoning ('why') separated from technical API documentation.

## `SOLOADER_UTILS_H` (line ~1)

**Source File:** `source/utils/utils.h`

> Copyright (C) 2021      Rinnegatamante
> Copyright (C) 2022-2024 Volodymyr Atamanenko
>
> This software may be modified and distributed under the terms
> of the MIT license. See the LICENSE file for details.

---

## `current_timestamp_ms` (line ~25)

**Source File:** `source/utils/utils.h`

> Get Unix timestamp in milliseconds.
>
> @return Number of milliseconds that have elapsed since January 1, 1970.

---

## `utils.h` (line ~32) (line ~32)

**Source File:** `source/utils/utils.h`

> Create a copy of a file.
>
> If the file specified by `destination` already exists, it will be
> overwritten. If a parent directory or directories of the file specified by
> `destination` do not exist, they will be created automatically.
>
> @warning The function will fail if the size of the source file specified by
>          `path` exceeds the amount of free memory available.
>
> @param[in] path        Full path of the source file.
> @param[in] destination Full path of the destination file.
>
> @return `true` on success, `false` otherwise.

---

## `utils.h` (line ~49) (line ~49)

**Source File:** `source/utils/utils.h`

> Check whether a file exists.
>
> @param path Full path of the file to look for.
>
> @return `true` if file exists, `false` otherwise.

---

## `utils.h` (line ~58) (line ~58)

**Source File:** `source/utils/utils.h`

> Load file contents into memory.
>
> @param[in]  path   Full path of the source file.
> @param[out] buffer Output buffer address, allocated by the function. Must be
>                    freed by the caller if the function returns `true`.
> @param[out] size   Output buffer size.
>
> @return `true` on success, `false` otherwise.

---

## `utils.h` (line ~70) (line ~70)

**Source File:** `source/utils/utils.h`

> Create directories leading to file.
>
> @param[in] path Full path of the target file.
> @param[in] mode Permissions to set for new directories (if any).
>
> @return `true` on success, `false` otherwise.

---

## `file_size` (line ~80)

**Source File:** `source/utils/utils.h`

> Save buffer contents into a file.
>
> @param[in] path   Full path of the target file.
> @param[in] buffer Buffer containing data to save.
> @param[in] size   Size of the buffer (in bytes).
>
> @return `true` on success, `false` otherwise.

---

## `file_size` (line ~91)

**Source File:** `source/utils/utils.h`

> Get the size of a file in bytes
>
> @param[in] path Full path of the target file.
>
> @return File size in bytes or (size_t)-1 in case of a failure.

---

## `file_sha1sum` (line ~100)

**Source File:** `source/utils/utils.h`

> Get SHA1 hash of file contents.
>
> @param[in] path Full path of the source file.
>
> @return 40-char long null-terminated string containing SHA1 hash. Can be
>         NULL in case of an error. Must be freed by the caller.

---

## `utils.h` (line ~110) (line ~110)

**Source File:** `source/utils/utils.h`

> Check whether specified path is a directory.
>
> @param[in] path Target path.
>
> @return `true` if path is a directory, `false` otherwise.

---

## `ret0` (line ~119)

**Source File:** `source/utils/utils.h`

> Check whether system module is loaded.
>
> @param[in] name Name of the system module to look for.
>
> @return `true` if the module is loaded, `false` otherwise.

---

## `ret0` (line ~128)

**Source File:** `source/utils/utils.h`

> Do nothing, return 0. Useful for stubbing.
> @return 0

---

## `ret1` (line ~134)

**Source File:** `source/utils/utils.h`

> Do nothing, return 1. Useful for stubbing.
> @return 1

---

## `retminus1` (line ~140)

**Source File:** `source/utils/utils.h`

> Do nothing, return -1. Useful for stubbing.
> @return -1

---

## `str_replace` (line ~146)

**Source File:** `source/utils/utils.h`

> Replace all occurrences of a substring in a string.
>
> @param[out] str         Target string. Will be extended using `realloc()` if
>                         the resulting string is longer than the initial one.
> @param[in]  needle      Substring to look for.
> @param[in]  replacement Replacement.

---

## `str_remove` (line ~156)

**Source File:** `source/utils/utils.h`

> Remove all occurrences of a substring from a string.
>
> @param[out] str    Target string.
> @param[in]  needle Substring to look for.

---

## `utils.h` (line ~164) (line ~164)

**Source File:** `source/utils/utils.h`

> Check whether a string starts with a substring.
>
> @param[in] str    Target string.
> @param[in] prefix Substring to look for.
>
> @return `true` if the string starts with the substring, `false` otherwise.

---

## `utils.h` (line ~174) (line ~174)

**Source File:** `source/utils/utils.h`

> Check whether a string ends with a substring.
>
> @param[in] str    Target string.
> @param[in] suffix Substring to look for.
>
> @return `true` if the string ends with the substring, `false` otherwise.

---

## `str_sha1sum` (line ~184)

**Source File:** `source/utils/utils.h`

> Get SHA1 hash of a string or byte array.
>
> @param[in] str  Source string or byte array.
> @param[in] size Length of the source string or byte array. If `0` is
>                 specified, `str` is treated as a null-terminated string.
>
> @return 40-char long null-terminated string containing SHA1 hash. Can be
>         NULL in case of an error. Must be freed by the caller.

---
