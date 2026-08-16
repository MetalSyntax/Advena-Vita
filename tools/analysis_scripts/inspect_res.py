import os

RES_DIR = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/Advena-1.0.1/res"
UX0_DIR = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/ux0_data/advena"

print("=== Subdirectories in res/ ===")
for root, dirs, files in os.walk(RES_DIR):
    rel = os.path.relpath(root, RES_DIR)
    png_files = [f for f in files if f.endswith(".png") or f.endswith(".xml") or f.endswith(".ogg")]
    if png_files:
        print(f"res/{rel}: {len(png_files)} files ({png_files[:5]}...)")

print("\n=== Current structure of ux0_data/advena ===")
for root, dirs, files in os.walk(UX0_DIR):
    rel = os.path.relpath(root, UX0_DIR)
    print(f"ux0_data/advena/{rel}: {len(files)} files")
