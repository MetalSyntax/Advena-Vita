import subprocess

result = subprocess.run(
    ["/Users/metalsyntax/vitasdk/bin/arm-vita-eabi-objdump", "-d", "-M", "force-thumb", "--start-address=0x13646c", "--stop-address=0x13648c", "/Volumes/Seagate/PSVITA Develop/Advena-Vita/ux0_data/advena/libgameDSO.so"],
    capture_output=True, text=True
)
print(result.stdout)
