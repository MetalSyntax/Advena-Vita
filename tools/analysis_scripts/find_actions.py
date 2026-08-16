import re

GHIDRA_FILE = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c"

with open(GHIDRA_FILE, "r") as f:
    lines = f.readlines()

# Search for CPlayer, CBattle, Jump, Teamstrike in Ghidra
for i, line in enumerate(lines):
    if "Jump" in line or "jump" in line or "Attack" in line or "TeamStrike" in line or "Teamstrike" in line:
        if "::" in line and line.strip().startswith("//"):
            print(f"L{i+1}: {line.strip()[:140]}")
