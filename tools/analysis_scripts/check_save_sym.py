import subprocess

result = subprocess.run(
    ["/Users/metalsyntax/vitasdk/bin/arm-vita-eabi-readelf", "--dyn-syms", "/Volumes/Seagate/PSVITA Develop/Advena-Vita/ux0_data/advena/libgameDSO.so"],
    capture_output=True, text=True
)

for line in result.stdout.splitlines():
    if "IsLeaglSaveData" in line or "LoadPlayData" in line:
        print(line)
