# `source/font.h` — Design Architecture & Notes

Explanatory and architectural design notes extracted from source code and replaced with concise technical Doxygen blocks. This document preserves the reasoning ('why') separated from technical API documentation.

## `gfa_font_init` (line ~6)

**Source File:** `source/font.h`

> Backend de rasterizado de fuente para el puente GFA (NexusFont.java) --
> reemplaza a android.graphics.Paint/Canvas/Bitmap con stb_truetype sobre
> app0:font.ttf (NanumGothic, OFL: cobertura Hangul completa + Latin, el
> juego usa strings coreanos via GFA_SetString/SetStringFromKSC5601).
>
> Convenciones (calcadas de NexusFont.java, que es la fuente de verdad):
> - "px" es Paint.setTextSize (tamano EM en pixeles).
> - ascent/descent son POSITIVOS y con ceil (GFA_GetAscent = -ceil(ascent()),
> GFA_GetDescent = ceil(descent())).
> - El buffer de pixeles es ARGB de 32 bits; el motor consume SOLO el canal
> alfa (byte >>24) para el cache de glifos (CopyPixelsToCharCacheBuffer,
> out_ghidra.c:153389) con stride = ancho del bitmap de GFA_Init.

---

## `gfa_font_advance` (line ~25)

**Source File:** `source/font.h`

> Ancho de avance de un codepoint / de un string de codepoints

---

## `gfa_font_break_text` (line ~29)

**Source File:** `source/font.h`

> Paint.breakText(text, true, maxWidth, null): cantidad de caracteres desde
> el inicio cuyo avance acumulado entra en maxWidth.

---

## `bw` (line ~33)

**Source File:** `source/font.h`

> Dibuja una linea de texto en buf (bw*bh pixeles ARGB), pen inicial en
> pen_x, baseline en baseline_y. color es ARGB no premultiplicado (el alfa
> del color escala la cobertura, igual que Paint.setColor). Devuelve el
> ancho avanzado en px.

---
