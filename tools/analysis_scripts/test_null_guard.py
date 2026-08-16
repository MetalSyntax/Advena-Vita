import subprocess

# Let's assemble:
# cmp r0, #0
# it eq
# bxeq lr
# cmp r1, #0
# it eq
# bxeq lr

asm = """
.syntax unified
.thumb
cmp r0, #0
it eq
bxeq lr
cmp r1, #0
it eq
bxeq lr
"""

with open("/tmp/test_guard.s", "w") as f:
    f.write(asm)

subprocess.run(["/Users/metalsyntax/vitasdk/bin/arm-vita-eabi-as", "-mthumb", "/tmp/test_guard.s", "-o", "/tmp/test_guard.o"])
res = subprocess.run(["/Users/metalsyntax/vitasdk/bin/arm-vita-eabi-objdump", "-d", "/tmp/test_guard.o"], capture_output=True, text=True)
print(res.stdout)
