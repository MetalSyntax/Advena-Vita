import subprocess

result = subprocess.run(
    ["/Users/metalsyntax/vitasdk/bin/arm-vita-eabi-objdump", "-d", "/Volumes/Seagate/PSVITA Develop/Advena-Vita/libgameDSO.so"],
    capture_output=True, text=True
)

lines = result.stdout.splitlines()
for i, line in enumerate(lines):
    if "changeUIController" in line or "ShowBtn" in line:
        print(line)
        for j in range(max(0, i-2), min(len(lines), i+15)):
            print("  ", lines[j])
        break
