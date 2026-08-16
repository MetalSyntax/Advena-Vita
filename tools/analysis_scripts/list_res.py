import os

RES_DIR = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/Advena-1.0.1/res"

for root, dirs, files in os.walk(RES_DIR):
    for f in sorted(files):
        fpath = os.path.join(root, f)
        rel = os.path.relpath(fpath, RES_DIR)
        print(f"res/{rel} ({os.path.getsize(fpath)} bytes)")
