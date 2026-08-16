with open("/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c", "r") as f:
    lines = f.readlines()

def print_range(start, end):
    for i in range(start-1, min(end, len(lines))):
        print(f"L{i+1}: {lines[i]}", end="")

print("=== Java_com_gamevil_nexus2_Natives_handleCletEvent ===")
print_range(221155, 221190)

print("\n=== handleCletEvent internal ===")
print_range(231035, 231150)
