# `source/java.c` — Design Architecture & Notes

Explanatory and architectural design notes extracted from source code and replaced with concise technical Doxygen blocks. This document preserves the reasoning ('why') separated from technical API documentation.

## `java.c` (line ~1) (line ~1)

**Source File:** `source/java.c`

> java.c — FalsoJNI bindings and callbacks for Advena (PS Vita)

---

## `name` (line ~116)

**Source File:** `source/java.c`

> JNI Handlers

---

## `java.c` (line ~192) (line ~192)

**Source File:** `source/java.c`

> On real Android this mirrors Context.getFilesDir().getAbsolutePath()
> (see Natives.getAbsolueFilePath()/NexusUtils.getAbsolueFilePath() in
> the decompiled APK). The native engine calls this JNI method directly
> (not through advena_resolve_asset_path) to build fopen() paths for its
> own private read/write state: save slots (s0.dat/s1.dat/s2.dat),
> game/options data (g.dat/g_an_g.dat/op.dat) and on-screen UI layout
> files (_uiButton_N/_uiDpad) — confirmed via logs/advena_latest.log and
> strings(1) on libgameDSO.so. Asset loading is handled separately by
> advena_resolve_asset_path()/Advena_readAssets(), which never consults
> this path, so it is safe to point this at the saves/ directory.

---

## `resolve_save_path` (line ~304)

**Source File:** `source/java.c`

> Save file handlers (ux0:data/advena/saves/<name>)

---

## `GFA_MAX_FONTS` (line ~397)

**Source File:** `source/java.c`

> Font GFA Handlers

---

## `java.c` (line ~550) (line ~550)

**Source File:** `source/java.c`

> Method ID Table

---
