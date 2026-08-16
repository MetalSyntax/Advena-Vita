import re

GHIDRA_FILE = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c"

with open(GHIDRA_FILE, "r") as f:
    for i, line in enumerate(f, 1):
        if "GVUI" in line or "Dpad" in line or "DPAD" in line or "dpad" in line or "VirtualKey" in line:
            if "::" in line:
                print(f"L{i}: {line.strip()[:140]}")
