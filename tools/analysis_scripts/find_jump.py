import re

GHIDRA_FILE = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c"

with open(GHIDRA_FILE, "r") as f:
    lines = f.readlines()

for i, l in enumerate(lines):
    if "SetJump" in l or "CB15InputKey::Get" in l or "CB15InputKey::Press" in l:
        print(f"L{i+1}: {l.strip()[:140]}")
