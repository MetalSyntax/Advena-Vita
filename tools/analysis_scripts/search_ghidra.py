import re

GHIDRA_FILE = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c"

with open(GHIDRA_FILE, "r", encoding="utf-8", errors="ignore") as f:
    lines = f.readlines()

print(f"Total lines in out_ghidra.c: {len(lines)}")

# Search for handleCletEvent or Java_
for i, line in enumerate(lines):
    if "Java_" in line or "handleClet" in line or "NativeInit" in line or "Teamstrike" in line or "Team" in line or "team" in line or "strike" in line or "Strike" in line:
        if any(w in line for w in ["Java_", "handleCletEvent", "NativeInit", "Teamstrike", "TeamStrike", "teamstrike"]):
            print(f"L{i+1}: {line.strip()[:150]}")
