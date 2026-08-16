with open("/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c", "r") as f:
    for i, line in enumerate(f, 1):
        if "CB15InputKey" in line and "Singleton" in line:
            print(f"L{i}: {line.strip()[:140]}")
