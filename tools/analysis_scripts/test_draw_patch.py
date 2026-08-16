import subprocess

asm = """
.syntax unified
.thumb
cmp r0, #0
it eq
bxeq lr
cmp r1, #0
it eq
bxeq lr
push {r4, r5, r6, r7, lr}
mov r7, r11
mov r6, r10
mov r5, r9
mov r4, r8
push {r4, r5, r6, r7}
sub sp, #76
"""

with open("/tmp/test_draw_entry.s", "w") as f:
    f.write(asm)

subprocess.run(["/Users/metalsyntax/vitasdk/bin/arm-vita-eabi-as", "-mthumb", "/tmp/test_draw_entry.s", "-o", "/tmp/test_draw_entry.o"])
res = subprocess.run(["/Users/metalsyntax/vitasdk/bin/arm-vita-eabi-objdump", "-d", "/tmp/test_draw_entry.o"], capture_output=True, text=True)
print(res.stdout)
