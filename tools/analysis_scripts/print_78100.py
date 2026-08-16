with open("/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c", "r") as f:
    lines = f.readlines()

print("=== 78100-78150 ===")
for i in range(78100, 78150):
    print(f"L{i+1}: {lines[i]}", end="")

print("=== 78970-79010 ===")
for i in range(78970, 79010):
    print(f"L{i+1}: {lines[i]}", end="")
