with open("/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c", "r") as f:
    for i, line in enumerate(f, 1):
        if "OnEvent(" in line or "g_pGxPointerPos" in line or "::OnEvent" in line:
            print(f"L{i}: {line.strip()[:140]}")
