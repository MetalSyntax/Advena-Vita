# `source/reimpl/pthr.h` — Design Architecture & Notes

Explanatory and architectural design notes extracted from source code and replaced with concise technical Doxygen blocks. This document preserves the reasoning ('why') separated from technical API documentation.

## `SOLOADER_PTHR_H` (line ~1)

**Source File:** `source/reimpl/pthr.h`

> Copyright (C) 2021      Andy Nguyen
> Copyright (C) 2022      Rinnegatamante
> Copyright (C) 2022      GrapheneCt
> Copyright (C) 2022-2023 Volodymyr Atamanenko
>
> This software may be modified and distributed under the terms
> of the MIT license. See the LICENSE file for details.

---

## `pthread_create_soloader` (line ~44)

**Source File:** `source/reimpl/pthr.h`

> pthread_t is same size on bionic and newlib

---

## `pthread_getschedparam_soloader` (line ~53)

**Source File:** `source/reimpl/pthr.h`

> pthread_t and sched_param are same size on bionic and newlib

---

## `pthread_condattr_init_soloader` (line ~57)

**Source File:** `source/reimpl/pthr.h`

> condattr_t is same size on bionic and newlib

---

## `pthread_mutexattr_init_soloader` (line ~61)

**Source File:** `source/reimpl/pthr.h`

> mutexattr_t is same size on bionic and newlib

---
