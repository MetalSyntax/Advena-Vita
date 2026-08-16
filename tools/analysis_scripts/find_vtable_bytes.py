SO_PATH = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/ux0_data/advena/libgameDSO.so"

with open(SO_PATH, "rb") as f:
    so_data = f.read()

# Let's search for 0x18cbe0 (vtable pointer in little-endian: E0 CB 18 00)
pos = 0
while True:
    pos = so_data.find(b"\xe0\xcb\x18\x00", pos)
    if pos == -1:
        break
    print(f"Found vtable pointer at offset 0x{pos:06x}")
    pos += 4
