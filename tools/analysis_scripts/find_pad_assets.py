import os

for root, dirs, files in os.walk("/Volumes/Seagate/PSVITA Develop/Advena-Vita/ux0_data/advena"):
    for f in files:
        if "pad" in f.lower() or "dpad" in f.lower() or "btn" in f.lower() or "team" in f.lower() or "strike" in f.lower() or ".pzx" in f.lower():
            print(os.path.join(root, f))
