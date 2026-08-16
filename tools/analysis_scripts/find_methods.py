import re

GHIDRA_FILE = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c"

with open(GHIDRA_FILE, "r") as f:
    for i, line in enumerate(f, 1):
        if line.startswith("// CBattleUI::") or line.startswith("// CB15InputKey::") or line.startswith("// CGameET::"):
            print(f"L{i}: {line.strip()}")
