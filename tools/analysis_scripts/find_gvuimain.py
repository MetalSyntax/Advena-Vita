with open("/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c", "r") as f:
    for i, line in enumerate(f, 1):
        if "GVUIMainSystem::" in line or "GVUISystem::" in line or "GVUIButton::" in line:
            if line.strip().startswith("//"):
                print(f"L{i}: {line.strip()[:140]}")
