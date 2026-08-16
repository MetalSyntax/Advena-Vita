import re

GHIDRA_FILE = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c"

terms = ["CBattleUI", "CB15InputKey", "CGameET", "Teamstrike", "teamstrike", "TeamStrike", "Team", "team_strike", "Tag", "tag", "Party", "party", "NativeInitDeviceInfo", "NativeResize", "illusiaDpad", "TouchOemIME"]

with open(GHIDRA_FILE, "r") as f:
    for i, line in enumerate(f, 1):
        for t in ["CBattleUI", "CB15InputKey", "CCharObject", "CGameET"]:
            if t in line:
                if "::" in line:
                    print(f"L{i}: {line.strip()[:140]}")
