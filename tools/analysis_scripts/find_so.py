import os

for root, dirs, files in os.walk("/Volumes/Seagate/PSVITA Develop/Advena-Vita"):
    for f in files:
        if f.endswith(".so"):
            print(os.path.join(root, f))
