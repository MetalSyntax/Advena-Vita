import subprocess

# Let's write 2 bytes to a temp file and disassemble with objdump
with open("/tmp/test_thumb.bin", "wb") as f:
    f.write(b"\x1f\x26") # Little-endian for 0x261f

result = subprocess.run(
    ["/Users/metalsyntax/vitasdk/bin/arm-vita-eabi-objdump", "-D", "-b", "binary", "-m", "arm", "-M", "force-thumb", "/tmp/test_thumb.bin"],
    capture_output=True, text=True
)
print(result.stdout)
