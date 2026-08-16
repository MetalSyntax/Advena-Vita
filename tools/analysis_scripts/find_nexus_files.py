import os

for root, dirs, files in os.walk("/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/apk_jadx"):
    for f in files:
        if "Nexus" in f or "Renderer" in f or "Activity" in f:
            print(os.path.join(root, f))
