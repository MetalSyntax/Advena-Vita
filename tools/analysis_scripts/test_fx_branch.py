import subprocess

asm = """
.syntax unified
.thumb
.org 0x136480
cmp r1, #0
beq target
str r4, [sp, #20]
.org 0x136496
target:
nop
"""

with open("/tmp/test_fx_branch.s", "w") as f:
    f.write(asm)

subprocess.run(["/Users/metalsyntax/vitasdk/bin/arm-vita-eabi-as", "-mthumb", "/tmp/test_fx_branch.s", "-o", "/tmp/test_fx_branch.o"])
res = subprocess.run(["/Users/metalsyntax/vitasdk/bin/arm-vita-eabi-objdump", "-d", "/tmp/test_fx_branch.o"], capture_output=True, text=True)
print(res.stdout)
