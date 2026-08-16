import os
import re

DECOMPILED = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled"
GHIDRA_FILE = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c"

def search_text(path, pattern, ext=None):
    prog = re.compile(pattern, re.IGNORECASE)
    for root, dirs, files in os.walk(path):
        for f in files:
            if ext and not f.endswith(ext):
                continue
            fpath = os.path.join(root, f)
            try:
                with open(fpath, "r", encoding="utf-8", errors="ignore") as fp:
                    for i, line in enumerate(fp, 1):
                        if prog.search(line):
                            print(f"{fpath}:{i}: {line.strip()[:140]}")
            except Exception as e:
                pass

if __name__ == "__main__":
    import sys
    query = sys.argv[1] if len(sys.argv) > 1 else "handleCletEvent"
    target = sys.argv[2] if len(sys.argv) > 2 else DECOMPILED
    search_text(target, query)
