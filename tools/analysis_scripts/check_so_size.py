import os

SO_PATH = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/ux0_data/advena/libgameDSO.so"
print(f"File size: {os.path.getsize(SO_PATH)} bytes (0x{os.path.getsize(SO_PATH):x})")
