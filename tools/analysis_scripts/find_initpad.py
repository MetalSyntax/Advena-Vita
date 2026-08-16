with open("/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c", "r") as f:
    lines = f.readlines()

def print_range(start, end):
    for i in range(start-1, min(end, len(lines))):
        print(f"L{i+1}: {lines[i]}", end="")

# Find where InitDPad, InitBattle etc are defined
for i, l in enumerate(lines):
    if "InitDPad" in l or "InitBattle" in l or "InitParty" in l or "InitSys" in l or "InitQuickSlot" in l or "CheckCircleKeyPad" in l:
        if l.strip().startswith("//"):
            print(f"L{i+1}: {l.strip()[:140]}")
