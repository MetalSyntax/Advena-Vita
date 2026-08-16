import struct

with open("/Volumes/Seagate/PSVITA Develop/Advena-Vita/ux0_data/advena/libgameDSO.so", "rb") as f:
    f.seek(0x190e70)
    data = f.read(0x60)

for i in range(0, len(data), 4):
    val = struct.unpack("<I", data[i:i+4])[0]
    print(f"0x{0x190e70 + i:x}: 0x{val:08x}")
