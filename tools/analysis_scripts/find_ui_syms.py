import subprocess

result = subprocess.run(
    ["/Users/metalsyntax/vitasdk/bin/arm-vita-eabi-nm", "-C", "/Volumes/Seagate/PSVITA Develop/Advena-Vita/libgameDSO.so"],
    capture_output=True, text=True
)

for line in result.stdout.splitlines():
    if "changeUIController" in line or "ShowBtn" in line or "InitialPlayerPadSet" in line:
        print(line)
