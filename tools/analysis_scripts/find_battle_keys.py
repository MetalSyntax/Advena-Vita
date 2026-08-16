import re

GHIDRA_FILE = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c"

with open(GHIDRA_FILE, "r") as f:
    for i, line in enumerate(f, 1):
        if "0x35" in line or "0x30" in line or "-5" in line:
            if "case" in line or "if" in line or "SetEventKey" in line:
                print(f"L{i}: {line.strip()[:140]}")
