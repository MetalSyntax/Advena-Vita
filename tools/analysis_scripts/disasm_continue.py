import subprocess

result = subprocess.run(
    ["/Users/metalsyntax/vitasdk/bin/arm-vita-eabi-objdump", "-d", "--start-address=0xff5ac", "--stop-address=0xff61c", "/Volumes/Seagate/PSVITA Develop/Advena-Vita/ux0_data/advena/libgameDSO.so"],
    capture_output=True, text=True
)
print(result.stdout)
