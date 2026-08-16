with open("/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c", "r") as f:
    for i, line in enumerate(f, 1):
        if "Java_com_gamevil_nexus2_Natives_NativeInit" in line or "Java_com_gamevil_nexus2_Natives_NativeResize" in line:
            print(f"L{i}: {line.strip()[:140]}")
