with open("/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c", "r") as f:
    lines = f.readlines()

for i in range(30600, 30685):
    print(f"L{i+1}: {lines[i]}", end="")
