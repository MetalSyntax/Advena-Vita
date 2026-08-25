# `source/main.c` — Design Architecture & Notes

Explanatory and architectural design notes extracted from source code and replaced with concise technical Doxygen blocks. This document preserves the reasoning ('why') separated from technical API documentation.

## `main.c` (line ~1) (line ~1)

**Source File:** `source/main.c`

> main.c — Main entry point and game loop for Advena PS Vita port

---

## `sceUserMainThreadStackSize` (line ~32)

**Source File:** `source/main.c`

> The process's main thread (the one the .so actually runs game logic and
> UI construction on -- shows up in crash dumps as thread "ADVENA001",
> named after the app's own VITA_TITLEID) is created by the SDK's own
> startup code BEFORE main() runs, using whatever size this global says --
> NOT the pthread_create_soloader() path in reimpl/pthr.c (that only
> covers threads the .so spawns itself via pthread_create). Left
> undeclared, it defaults to a small stack (256KB) that is nowhere near
> enough for this engine's deep init/UI call chains (e.g.
> GVUIPlayerController construction -> InitialPlayerPadSet), causing a
> Data Abort stack overflow confirmed via psp2dmp (fault on a plain
> stack-store instruction in a function's own prologue).

---

## `GAME_W` (line ~45)

**Source File:** `source/main.c`

> Advena native logical UI/canvas resolution: 480x320 (as defined in
> AdvenaLauncher.java: gameScreenWidth = 480, gameScreenHeight = 320).
> The software rasterizer renders to this 480x320 buffer, and glResize
> scales it to fill the Vita's 960x544 screen.

---

## `KEY_WALK_UP` (line ~62)

**Source File:** `source/main.c`

> Directional Movement KeyCodes (Official Nexus2 / Advena Hal KeyCodes)

---

## `DPAD_CENTER_X` (line ~100)

**Source File:** `source/main.c`

> Virtual Touch Hotspots (in 480x320 native design coordinates)

---

## `BTN_TARGET_X` (line ~107)

**Source File:** `source/main.c`

> Left Teamstrike / Quest / Target / "!" button (Button 0 in GVUIPlayerController)

---

## `BTN_CHAR_SWITCH_X` (line ~111)

**Source File:** `source/main.c`

> Right Character Switch Cycle button (Button 1 in GVUIPlayerController)

---

## `main.c` (line ~156) (line ~156)

**Source File:** `source/main.c`

> Dedicated Pointer IDs for each virtual button (1 to 12)

---

## `main_thread_id` (line ~254)

**Source File:** `source/main.c`

> Pin the main thread (render + game logic, single NativeRender() tick
> per loop iteration) to its own core so the scheduler never migrates it
> between cores while the audio mixer (audio.c, see audio_init) runs
> fixed on a different one. The 3 user cores available are
> SCE_KERNEL_CPU_MASK_USER_0/1/2 (the 4th is reserved by the system).
> Unconditional, no build flag gate -- same criterion as the CPU/GPU/bus
> clock boost above, not a risky experiment. Technique validated on real
> hardware in the sibling Zenonia 4 port (same Gamevil Nexus2/GxPZx
> engine family).

---

## `main.c` (line ~271) (line ~271)

**Source File:** `source/main.c`

> Native initialization (400x240 native UI/logical canvas, 960x544 Vita display).
> This matches the config used since the very first working commit
> (26074c4) -- confirmed by diffing against it -- so it is not the
> source of the crop/zoom regression (see GAME_W/GAME_H comment above).

---

## `main.c` (line ~290) (line ~290)

**Source File:** `source/main.c`

> Language configuration: English (1)

---

## `MAX_TOUCH_SLOTS` (line ~307)

**Source File:** `source/main.c`

> Hardware touch slot tracker (max 5 slots)

---

## `seen` (line ~321)

**Source File:** `source/main.c`

> 1. Handle Physical Touch Screen Input (with stable slot tracking)

---

## `pad` (line ~367)

**Source File:** `source/main.c`

> 2. Handle Physical Buttons & Analog Sticks

---

## `up_pressed` (line ~371)

**Source File:** `source/main.c`

> Directional handling (D-Pad & Left Stick -> Send Walk KeyCodes 50, 56, 52, 54)

---

## `cross_down` (line ~414)

**Source File:** `source/main.c`

> Cross (X) -> Attack / NPC Talk / Action / Confirm (Key 53)

---

## `circle_down` (line ~425)

**Source File:** `source/main.c`

> Circle (O) -> Right / Forward Jump in battle (Key -4)

---

## `triangle_down` (line ~436)

**Source File:** `source/main.c`

> Triangle (Δ) -> Left / Backward Jump in battle (Key -3)

---

## `rstick_left` (line ~447)

**Source File:** `source/main.c`

> Right Stick: Quick cast skills 1, 2, 3, 4

---

## `square_down` (line ~453)

**Source File:** `source/main.c`

> Square (□) / Right Stick Left -> Skill 1 (Key -13)

---

## `l1_down` (line ~493)

**Source File:** `source/main.c`

> L1 Trigger -> Teamstrike (T Button) (Key 48)

---

## `r1_down` (line ~504)

**Source File:** `source/main.c`

> R1 Trigger -> Character Tag / Swap (Key -12)

---

## `select_down` (line ~515)

**Source File:** `source/main.c`

> Select -> Cancel / Back / Inventory in menus (-16)

---

## `main.c` (line ~543) (line ~543)

**Source File:** `source/main.c`

> No explicit sceDisplayWaitVblankStartMulti() here: vglSwapBuffers()
> (in gl_swap, glutil.c) already performs the display flip/vblank
> sync internally. A second, unconditional 2-vblank wait stacked on
> top of that was capping every frame at ~33.3ms (30 FPS) regardless
> of how fast the frame actually rendered -- e.g. a 17ms frame still
> paid the full 33.3ms wait, landing at 20 FPS instead of the ~50 FPS
> it could have sustained. Removing it lets light scenes (menus,
> simple maps) run up to the panel's native 60Hz; it does not change
> heavy combat scenes that are already GL-bound below 30 FPS (see
> PORTING_PLAN.md Bug 16 -- that bottleneck is draw-call/state-change
> volume, unrelated to this pacing wait, and is still pending the
> glDrawArrays/glDrawElements/glBindTexture instrumentation below).

---
