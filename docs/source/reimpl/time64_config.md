# `source/reimpl/time64_config.h` — Design Architecture & Notes

Explanatory and architectural design notes extracted from source code and replaced with concise technical Doxygen blocks. This document preserves the reasoning ('why') separated from technical API documentation.

## `INT_64_T` (line ~1)

**Source File:** `source/reimpl/time64_config.h`

> Debugging
> TIME_64_DEBUG
> Define if you want debugging messages

---

## `INT_64_T` (line ~5)

**Source File:** `source/reimpl/time64_config.h`

> #define TIME_64_DEBUG

---

## `INT_64_T` (line ~8)

**Source File:** `source/reimpl/time64_config.h`

> INT_64_T
> A 64 bit integer type to use to store time and others.
> Must be defined.

---

## `time64_config.h` (line ~15) (line ~15)

**Source File:** `source/reimpl/time64_config.h`

> USE_TM64
> Should we use a 64 bit safe replacement for tm?  This will
> let you go past year 2 billion but the struct will be incompatible
> with tm.  Conversion functions will be provided.

---

## `time64_config.h` (line ~20) (line ~20)

**Source File:** `source/reimpl/time64_config.h`

> #define USE_TM64

---

## `HAS_GMTIME_R` (line ~23)

**Source File:** `source/reimpl/time64_config.h`

> Availability of system functions.
>
> HAS_GMTIME_R
> Define if your system has gmtime_r()
>
> HAS_LOCALTIME_R
> Define if your system has localtime_r()
>
> HAS_TIMEGM
> Define if your system has timegm(), a GNU extension.

---

## `USE_SYSTEM_LOCALTIME` (line ~39)

**Source File:** `source/reimpl/time64_config.h`

> Details of non-standard tm struct elements.
>
> HAS_TM_TM_GMTOFF
> True if your tm struct has a "tm_gmtoff" element.
> A BSD extension.
>
> HAS_TM_TM_ZONE
> True if your tm struct has a "tm_zone" element.
> A BSD extension.

---

## `USE_SYSTEM_LOCALTIME` (line ~49)

**Source File:** `source/reimpl/time64_config.h`

> #define HAS_TM_TM_GMTOFF
> #define HAS_TM_TM_ZONE

---

## `USE_SYSTEM_LOCALTIME` (line ~53)

**Source File:** `source/reimpl/time64_config.h`

> USE_SYSTEM_LOCALTIME
> USE_SYSTEM_GMTIME
> Should we use the system functions if the time is inside their range?
> Your system localtime() is probably more accurate, but our gmtime() is
> fast and safe.

---

## `SYSTEM_LOCALTIME_MAX` (line ~60)

**Source File:** `source/reimpl/time64_config.h`

> #define USE_SYSTEM_GMTIME

---

## `SYSTEM_LOCALTIME_MAX` (line ~63)

**Source File:** `source/reimpl/time64_config.h`

> SYSTEM_LOCALTIME_MAX
> SYSTEM_LOCALTIME_MIN
> SYSTEM_GMTIME_MAX
> SYSTEM_GMTIME_MIN
> Maximum and minimum values your system's gmtime() and localtime()
> can handle.  We will use your system functions if the time falls
> inside these ranges.

---
