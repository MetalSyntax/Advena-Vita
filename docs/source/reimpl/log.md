# `source/reimpl/log.h` — Design Architecture & Notes

Explanatory and architectural design notes extracted from source code and replaced with concise technical Doxygen blocks. This document preserves the reasoning ('why') separated from technical API documentation.

## `SOLOADER_LOG_H` (line ~1)

**Source File:** `source/reimpl/log.h`

> Copyright (C) 2009      The Android Open Source Project
> Copyright (C) 2021      Andy Nguyen
> Copyright (C) 2022      Rinnegatamante
> Copyright (C) 2022-2023 Volodymyr Atamanenko
>
> This software may be modified and distributed under the terms
> of the MIT license. See the LICENSE file for details.

---

## `log.h` (line ~25) (line ~25)

**Source File:** `source/reimpl/log.h`

> Android log priority values, in increasing order of priority.

---

## `log.h` (line ~29) (line ~29)

**Source File:** `source/reimpl/log.h`

> For internal use only.

---

## `log.h` (line ~31) (line ~31)

**Source File:** `source/reimpl/log.h`

> The default priority, for internal use only.

---

## `log.h` (line ~33) (line ~33)

**Source File:** `source/reimpl/log.h`

> Verbose logging. Should typically be disabled for a release apk.

---

## `log.h` (line ~35) (line ~35)

**Source File:** `source/reimpl/log.h`

> Debug logging. Should typically be disabled for a release apk.

---

## `log.h` (line ~37) (line ~37)

**Source File:** `source/reimpl/log.h`

> Informational logging. Should typically be disabled for a release apk.

---

## `log.h` (line ~39) (line ~39)

**Source File:** `source/reimpl/log.h`

> Warning logging. For use with recoverable failures.

---

## `__android_log_write` (line ~41)

**Source File:** `source/reimpl/log.h`

> Error logging. For use with unrecoverable failures.

---

## `__android_log_write` (line ~43)

**Source File:** `source/reimpl/log.h`

> Fatal logging. For use when aborting.

---

## `__android_log_write` (line ~45)

**Source File:** `source/reimpl/log.h`

> For internal use only.

---

## `__android_log_write` (line ~49)

**Source File:** `source/reimpl/log.h`

> Writes the constant string `text` to the log, with priority `prio` and tag
> `tag`.

---

## `__android_log_print` (line ~55)

**Source File:** `source/reimpl/log.h`

> Writes a formatted string to the log, with priority `prio` and tag `tag`.
> The details of formatting are the same as for
> [printf(3)](http://man7.org/linux/man-pages/man3/printf.3.html).

---

## `__android_log_vprint` (line ~63)

**Source File:** `source/reimpl/log.h`

> Equivalent to `__android_log_print`, but taking a `va_list`.
> (If `__android_log_print` is like `printf`, this is like `vprintf`.)

---

## `__android_log_assert` (line ~70)

**Source File:** `source/reimpl/log.h`

> Writes an assertion failure to the log (as `ANDROID_LOG_FATAL`) and to
> stderr, before calling
> [abort(3)](http://man7.org/linux/man-pages/man3/abort.3.html).
>
> If `fmt` is non-null, `cond` is unused. If `fmt` is null, the string
> `Assertion failed: %s` is used with `cond` as the string argument.
> If both `fmt` and `cond` are null, a default string is provided.
>
> Most callers should use
> [assert(3)](http://man7.org/linux/man-pages/man3/assert.3.html) from
> `&lt;assert.h&gt;` instead, or the `__assert` and `__assert2` functions
> provided by bionic if more control is needed. They support automatically
> including the source filename and line number more conveniently than this
> function.

---
