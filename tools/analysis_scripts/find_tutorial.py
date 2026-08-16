with open("/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c", "r") as f:
    lines = f.readlines()

for i, l in enumerate(lines):
    if "CUITotorial" in l or "Tutorial" in l or "Totorial" in l:
        if l.strip().startswith("//"):
            print(f"L{i+1}: {l.strip()[:140]}")
