import re

GHIDRA_FILE = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c"

with open(GHIDRA_FILE, "r") as f:
    for i, line in enumerate(f, 1):
        if "Team" in line or "team" in line or "Strike" in line or "strike" in line or "Party" in line:
            if "::" in line and line.strip().startswith("//"):
                print(f"L{i}: {line.strip()[:140]}")
