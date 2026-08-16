import struct

SO_PATH = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/ux0_data/advena/libgameDSO.so"

with open(SO_PATH, "rb") as f:
    so_data = f.read()

offset = 0x184be0
print(f"Bytes at 0x{offset:06x}: {so_data[offset:offset+32].hex()}")
vtable_entries = struct.unpack("<8I", so_data[offset:offset+32])
for i, entry in enumerate(vtable_entries):
    print(f"  vtable[{i}] = 0x{entry:08x}")
