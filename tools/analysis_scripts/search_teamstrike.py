import re

GHIDRA_FILE = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c"

with open(GHIDRA_FILE, "r") as f:
    for i, line in enumerate(f, 1):
        if any(w in line for w in ["TeamStrike", "Teamstrike", "teamstrike", "team_strike", "Team_Strike", "SetEventKey", "0x30"]):
            if any(w in line for w in ["Team", "team", "strike", "Strike", "0x30", "SetEventKey"]):
                print(f"L{i}: {line.strip()[:140]}")
