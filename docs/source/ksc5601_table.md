# `source/ksc5601_table.h` — Design Architecture & Notes

Explanatory and architectural design notes extracted from source code and replaced with concise technical Doxygen blocks. This document preserves the reasoning ('why') separated from technical API documentation.

## `KSC5601_TABLE_H` (line ~1)

**Source File:** `source/ksc5601_table.h`

> Tabla EUC-KR/CP949 (KSC5601) -> Unicode BMP, generada con Python codecs
> cp949. Indexada [(lead-0x81)*190 + (trail-0x41)]; 0 = secuencia invalida.
> Usada por GFA_SetStringFromKSC5601 (java.c) -- new String(data, "KSC5601").

---
