import subprocess

asm = """
.syntax unified
.thumb
.org 0x136076
cmp r6, #0
beq target
.org 0x136068
target:
nop
"""

with open("/tmp/test_branch.s", "w") as f:
    f.write(asm)

subprocess.run(["/Users/metalsyntax/vitasdk/bin/arm-vita-eabi-as", "-mthumb", "/tmp/test_branch.s", "-o", "/tmp/test_branch.o"])
res = subprocess.run(["/Users/metalsyntax/vitasdk/bin/arm-vita-eabi-objdump", "-d", "/tmp/test_branch.o"], capture_output=True, text=True)
print(res.stdout)
