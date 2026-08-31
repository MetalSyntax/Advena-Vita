/**
 * @brief main.c — Main entry point and game loop for Advena PS Vita port.
 * @note See `docs/source/main.md:1` for detailed design rationale.
 */

#include "utils/init.h"
#include "utils/glutil.h"
#include "utils/logger.h"
#include "utils/dialog.h"
#include "audio.h"
#include "java.h"

#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/display.h>
#include <psp2/power.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>
#include <vitaGL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int _newlib_heap_size_user = 128 * 1024 * 1024;

#ifdef USE_SCELIBC_IO
int sceLibcHeapSize = 4 * 1024 * 1024;
#endif

/**
 * @brief The process's main thread (the one the .so actually runs game logic and UI construction on -- shows up in crash dumps as thread).
 * @note See `docs/source/main.md:32` for detailed design rationale.
 */
unsigned int sceUserMainThreadStackSize = 4 * 1024 * 1024;

/**
 * @brief Advena native logical UI/canvas resolution.
 * @note See `docs/source/main.md:45` for detailed design rationale.
 */
#define GAME_W 480
#define GAME_H 320
#define SCREEN_W 960
#define SCREEN_H 544

#define ENGINE_LOGICAL_W 480
#define ENGINE_LOGICAL_H 320
#define EVENT_KEY_DOWN   2
#define EVENT_KEY_UP     3
#define EVENT_TOUCH_DOWN 23
#define EVENT_TOUCH_UP   24
#define EVENT_TOUCH_MOVE 25

/**
 * @brief Directional Movement KeyCodes (Official Nexus2 / Advena Hal KeyCodes).
 * @note See `docs/source/main.md:62` for detailed design rationale.
 */
#define KEY_WALK_UP         50    // '2' (0x32) -> Walk UP / Ladder UP
#define KEY_WALK_DOWN       56    // '8' (0x38) -> Walk DOWN / Ladder DOWN
#define KEY_WALK_LEFT       52    // '4' (0x34) -> Walk LEFT
#define KEY_WALK_RIGHT      54    // '6' (0x36) -> Walk RIGHT

// Battle Action KeyCodes
#define KEY_ATTACK          53    // '5' (0x35) -> Attack
#define KEY_JUMP_LEFT       (-3)  // Jump Left (Back jump / A)
#define KEY_JUMP_RIGHT      (-4)  // Jump Right (Forward jump / D)
#define KEY_TEAMSTRIKE      48    // '0' (0x30) -> Teamstrike (Button T) & Quest [!]
#define KEY_CHAR_SWITCH     (-12) // Tag / Switch Character (HAL_KEY_POWER)

// QuickSlot Skill KeyCodes
#define KEY_SKILL1          (-13) // Skill 1 (QuickSlot 1 / HAL_KEY_SIDE_UP)
#define KEY_SKILL2          42    // '*' (0x2a) -> Skill 2 (QuickSlot 2)
#define KEY_SKILL3          (-14) // Skill 3 (QuickSlot 3 / HAL_KEY_SIDE_DOWN)
#define KEY_SKILL4          35    // '#' (0x23) -> Skill 4 (QuickSlot 4)

// Menu / Navigation KeyCodes
#define KEY_MENU_OK         (-5)  // OK / Confirm (in menus)
#define KEY_MENU_SOFT1      (-6)  // Submenu
#define KEY_MENU_SOFT2      (-7)
#define KEY_MENU_PAUSE      (-8)  // Pause
#define KEY_MENU_BACK       (-16) // Cancel / Back / Inventory

so_module so_mod;

static int (* Game_JNI_OnLoad)(void *vm, void *reserved);
static void (* InitializeJNIGlobalRef)(void *env, void *obj);
static void (* NativeInitWithBufferSize)(void *env, void *obj, int w, int h);
static void (* NativeInitDeviceInfo)(void *env, void *obj, int w, int h);
static void (* NativeResize)(void *env, void *obj, int w, int h);
static void (* NativeRender)(void *env, void *obj);
static void (* NativeResumeClet)(void *env, void *obj);
static void (* NativePauseClet)(void *env, void *obj);
static void (* handleCletEvent)(void *env, void *obj, int type, int p1, int p2, int p3);

/**
 * @brief Virtual Touch Hotspots (in 480x320 native design coordinates).
 * @note See `docs/source/main.md:100` for detailed design rationale.
 */
#define DPAD_CENTER_X     72
#define DPAD_CENTER_Y     233
#define DPAD_RADIUS       50

/**
 * @brief Left Teamstrike / Quest / Target / ".
 * @note See `docs/source/main.md:107` for detailed design rationale.
 */
#define BTN_TARGET_X      29
#define BTN_TARGET_Y      128

/**
 * @brief Right Character Switch Cycle button (Button 1 in GVUIPlayerController).
 * @note See `docs/source/main.md:111` for detailed design rationale.
 */
#define BTN_CHAR_SWITCH_X 428
#define BTN_CHAR_SWITCH_Y 169

// Battle Action Controls
#define BTN_ATTACK_X      414
#define BTN_ATTACK_Y      247

#define BTN_JUMP_LEFT_X   366
#define BTN_JUMP_LEFT_Y   180

#define BTN_JUMP_RIGHT_X  414
#define BTN_JUMP_RIGHT_Y  153

// Bottom Skill Bar (4 slots)
#define BTN_SKILL1_X      216
#define BTN_SKILL1_Y      287

#define BTN_SKILL2_X      264
#define BTN_SKILL2_Y      287

#define BTN_SKILL3_X      312
#define BTN_SKILL3_Y      287

#define BTN_SKILL4_X      360
#define BTN_SKILL4_Y      287

// Top Bar Elements
#define BTN_PARTY1_X      54
#define BTN_PARTY1_Y      27

#define BTN_BAG_X         240
#define BTN_BAG_Y         27

#define BTN_PAUSE_X       456
#define BTN_PAUSE_Y       27

typedef struct {
    int active;
    int x;
    int y;
    int pointer_id;
    const char *name;
} TouchState;

/**
 * @brief Dedicated Pointer IDs for each virtual button (1 to 12).
 * @note See `docs/source/main.md:156` for detailed design rationale.
 */
static TouchState g_dpad_touch        = {0, 0, 0, 1, "D-Pad"};
static TouchState g_btn_attack        = {0, BTN_ATTACK_X, BTN_ATTACK_Y, 2, "Attack"};
static TouchState g_btn_jump_left     = {0, BTN_JUMP_LEFT_X, BTN_JUMP_LEFT_Y, 3, "Jump Left"};
static TouchState g_btn_jump_right    = {0, BTN_JUMP_RIGHT_X, BTN_JUMP_RIGHT_Y, 4, "Jump Right"};
static TouchState g_btn_teamstrike    = {0, BTN_TARGET_X, BTN_TARGET_Y, 5, "Teamstrike [T]"};
static TouchState g_btn_char_switch   = {0, BTN_CHAR_SWITCH_X, BTN_CHAR_SWITCH_Y, 6, "Char Switch"};
static TouchState g_btn_skill1        = {0, BTN_SKILL1_X, BTN_SKILL1_Y, 7, "Skill 1"};
static TouchState g_btn_skill2        = {0, BTN_SKILL2_X, BTN_SKILL2_Y, 8, "Skill 2"};
static TouchState g_btn_skill3        = {0, BTN_SKILL3_X, BTN_SKILL3_Y, 9, "Skill 3"};
static TouchState g_btn_skill4        = {0, BTN_SKILL4_X, BTN_SKILL4_Y, 10, "Skill 4"};
static TouchState g_btn_bag           = {0, BTN_BAG_X, BTN_BAG_Y, 11, "Bag"};
static TouchState g_btn_pause         = {0, BTN_PAUSE_X, BTN_PAUSE_Y, 12, "Pause"};

static void send_key_event(int keycode, int is_down) {
    if (!handleCletEvent) return;
    int event_type = is_down ? EVENT_KEY_DOWN : EVENT_KEY_UP;
    handleCletEvent(&jni, NULL, event_type, keycode, 0, 0);
}

static void update_virtual_button(TouchState *btn, int is_down) {
    if (!handleCletEvent) return;
    if (is_down && !btn->active) {
        btn->active = 1;
        l_info("[TOUCH_DOWN] %s at (%d, %d) ptr %d", btn->name, btn->x, btn->y, btn->pointer_id);
        handleCletEvent(&jni, NULL, EVENT_TOUCH_DOWN, btn->x, btn->y, btn->pointer_id);
    } else if (!is_down && btn->active) {
        btn->active = 0;
        l_info("[TOUCH_UP] %s at (%d, %d) ptr %d", btn->name, btn->x, btn->y, btn->pointer_id);
        handleCletEvent(&jni, NULL, EVENT_TOUCH_UP, btn->x, btn->y, btn->pointer_id);
    }
}

static void update_virtual_dpad(int dx, int dy) {
    if (!handleCletEvent) return;
    if (dx == 0 && dy == 0) {
        if (g_dpad_touch.active) {
            g_dpad_touch.active = 0;
            l_info("[DPAD_TOUCH_UP] (%d, %d) ptr %d", g_dpad_touch.x, g_dpad_touch.y, g_dpad_touch.pointer_id);
            handleCletEvent(&jni, NULL, EVENT_TOUCH_UP, g_dpad_touch.x, g_dpad_touch.y, g_dpad_touch.pointer_id);
        }
        return;
    }

    int tx = DPAD_CENTER_X + dx * DPAD_RADIUS;
    int ty = DPAD_CENTER_Y + dy * DPAD_RADIUS;

    if (!g_dpad_touch.active) {
        g_dpad_touch.active = 1;
        g_dpad_touch.x = tx;
        g_dpad_touch.y = ty;
        l_info("[DPAD_TOUCH_DOWN] (%d, %d) dx=%d dy=%d ptr %d", tx, ty, dx, dy, g_dpad_touch.pointer_id);
        handleCletEvent(&jni, NULL, EVENT_TOUCH_DOWN, tx, ty, g_dpad_touch.pointer_id);
    } else if (g_dpad_touch.x != tx || g_dpad_touch.y != ty) {
        g_dpad_touch.x = tx;
        g_dpad_touch.y = ty;
        handleCletEvent(&jni, NULL, EVENT_TOUCH_MOVE, tx, ty, g_dpad_touch.pointer_id);
    }
}

int main() {
    soloader_init_all();

    advena_install_array_hooks();

    Game_JNI_OnLoad = (void *)so_symbol(&so_mod, "JNI_OnLoad");
    InitializeJNIGlobalRef = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_InitializeJNIGlobalRef");
    NativeInitWithBufferSize = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeInitWithBufferSize");
    NativeInitDeviceInfo = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeInitDeviceInfo");
    NativeResize = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeResize");
    NativeRender = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeRender");
    NativeResumeClet = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeResumeClet");
    NativePauseClet = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativePauseClet");
    handleCletEvent = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_handleCletEvent");

    l_info("Game_JNI_OnLoad: %p", Game_JNI_OnLoad);
    l_info("InitializeJNIGlobalRef: %p", InitializeJNIGlobalRef);
    l_info("NativeInitWithBufferSize: %p", NativeInitWithBufferSize);
    l_info("NativeInitDeviceInfo: %p", NativeInitDeviceInfo);
    l_info("NativeResize: %p", NativeResize);
    l_info("NativeRender: %p", NativeRender);
    l_info("NativeResumeClet: %p", NativeResumeClet);
    l_info("NativePauseClet: %p", NativePauseClet);
    l_info("handleCletEvent: %p", handleCletEvent);

    if (Game_JNI_OnLoad) {
        l_info("Calling Game_JNI_OnLoad...");
        Game_JNI_OnLoad(&jvm, NULL);
    }

    if (InitializeJNIGlobalRef) {
        l_info("Calling InitializeJNIGlobalRef...");
        InitializeJNIGlobalRef(&jni, NULL);
    }

    l_info("Initializing OpenGL (%dx%d)...", SCREEN_W, SCREEN_H);
    gl_init(SCREEN_W, SCREEN_H);

    /**
     * @brief Pin the main thread (render + game logic, single NativeRender() tick per loop iteration) to its own core so the scheduler never migrates it.
     * @note See `docs/source/main.md:254` for detailed design rationale.
     */
    int main_thread_id = sceKernelGetThreadId();
    int affinity_res = sceKernelChangeThreadCpuAffinityMask(main_thread_id, SCE_KERNEL_CPU_MASK_USER_0);
    l_info("[PERF] CPU affinity: main thread (0x%08x) -> core 0 (res=0x%08x)", main_thread_id, affinity_res);

    // Audio initialization
    l_info("Initializing audio subsystem...");
    audio_init();

    /**
     * @brief Native initialization (400x240 native UI/logical canvas, 960x544 Vita display).
     * @note See `docs/source/main.md:271` for detailed design rationale.
     */
    if (NativeInitWithBufferSize) {
        l_info("Calling NativeInitWithBufferSize(%d, %d)...", ENGINE_LOGICAL_W, ENGINE_LOGICAL_H);
        NativeInitWithBufferSize(&jni, NULL, ENGINE_LOGICAL_W, ENGINE_LOGICAL_H);
    }

    if (NativeInitDeviceInfo) {
        l_info("Calling NativeInitDeviceInfo(%d, %d)...", ENGINE_LOGICAL_W, ENGINE_LOGICAL_H);
        NativeInitDeviceInfo(&jni, NULL, ENGINE_LOGICAL_W, ENGINE_LOGICAL_H);
    }

    if (NativeResize) {
        l_info("Calling NativeResize(%d, %d)...", SCREEN_W, SCREEN_H);
        NativeResize(&jni, NULL, SCREEN_W, SCREEN_H);
    }

    /**
     * @brief Language configuration.
     * @note See `docs/source/main.md:290` for detailed design rationale.
     */
    if (handleCletEvent) {
        l_info("Configuring language to English (GET_LANGUAGE = 1)...");
        handleCletEvent(&jni, NULL, 5001, 1, 0, 0); // GET_LANGUAGE = 1 (English)
    }

    if (NativeResumeClet) {
        l_info("Calling NativeResumeClet...");
        NativeResumeClet(&jni, NULL);
    }

    l_success("Advena loaded successfully. Entering main render loop.");

    // Enable hardware input sampling
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);

    /**
     * @brief Hardware touch slot tracker (max 5 slots), forwarding only ONE "primary" touch to
     * the engine at a time.
     * @note The native touch dispatch (handleCletEvent event types 23/24/25, out_ghidra.c:231038)
     * funnels every touch into a SINGLE global pointer-position struct and always notifies one
     * fixed event target -- it has no per-finger/pointer-id tracking at all, matching Android's
     * original single-touch design for this UI queue. Forwarding more than one simultaneous
     * hardware contact (e.g. a resting palm/thumb near a screen corner while holding the console,
     * right where corner UI elements like the dialogue Skip button sit) stomps that single global
     * position mid-gesture and can silently eat a genuine tap. Only the most recently started
     * touch is forwarded as "primary"; any other simultaneous contact is tracked (to detect
     * up/down transitions) but never sent to the engine.
     */
    #define MAX_TOUCH_SLOTS 5
    int last_touch_x[MAX_TOUCH_SLOTS] = {-1, -1, -1, -1, -1};
    int last_touch_y[MAX_TOUCH_SLOTS] = {-1, -1, -1, -1, -1};
    int slot_hw_id[MAX_TOUCH_SLOTS]   = {-1, -1, -1, -1, -1};
    int primary_slot = -1;
    int primary_fwd_x = -1, primary_fwd_y = -1;

    int old_up = 0, old_down = 0, old_left = 0, old_right = 0;
    int old_cross = 0, old_circle = 0, old_triangle = 0, old_square = 0;
    int old_l1 = 0, old_r1 = 0, old_select = 0, old_start = 0;
    int old_rstick_up = 0, old_rstick_down = 0, old_rstick_left = 0, old_rstick_right = 0;

    SceTouchData touch_data;

    while (1) {
        /**< @brief 1. Handle Physical Touch Screen Input (single "primary" touch forwarded). */
        sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch_data, 1);
        int seen[MAX_TOUCH_SLOTS] = {0};
        int new_slot = -1;

        for (int r = 0; r < touch_data.reportNum && r < MAX_TOUCH_SLOTS; r++) {
            int hw_id = touch_data.report[r].id;
            int x = (touch_data.report[r].x * GAME_W) / 1920;
            int y = (touch_data.report[r].y * GAME_H) / 1088;
            if (x < 0) x = 0; if (x >= GAME_W) x = GAME_W - 1;
            if (y < 0) y = 0; if (y >= GAME_H) y = GAME_H - 1;

            int slot = -1;
            for (int s = 0; s < MAX_TOUCH_SLOTS; s++) {
                if (slot_hw_id[s] == hw_id) { slot = s; break; }
            }
            if (slot == -1) {
                for (int s = 0; s < MAX_TOUCH_SLOTS; s++) {
                    if (slot_hw_id[s] == -1) { slot = s; break; }
                }
                if (slot == -1) continue;
                slot_hw_id[slot] = hw_id;
                new_slot = slot;
            }
            seen[slot] = 1;
            last_touch_x[slot] = x;
            last_touch_y[slot] = y;
        }

        for (int s = 0; s < MAX_TOUCH_SLOTS; s++) {
            if (slot_hw_id[s] != -1 && !seen[s]) {
                if (s == primary_slot) {
                    l_info("[HW_TOUCH_UP] Finger release at (%d, %d) slot %d", primary_fwd_x, primary_fwd_y, s);
                    if (handleCletEvent) handleCletEvent(&jni, NULL, EVENT_TOUCH_UP, primary_fwd_x, primary_fwd_y, 0);
                    primary_slot = -1;
                    primary_fwd_x = -1;
                    primary_fwd_y = -1;
                }
                slot_hw_id[s] = -1;
            }
        }

        /**< @brief A brand-new contact always takes over as the primary/forwarded touch. */
        if (new_slot != -1 && new_slot != primary_slot) {
            if (primary_slot != -1 && handleCletEvent) {
                l_info("[HW_TOUCH_UP] Primary touch pre-empted by a new contact, releasing (%d, %d)", primary_fwd_x, primary_fwd_y);
                handleCletEvent(&jni, NULL, EVENT_TOUCH_UP, primary_fwd_x, primary_fwd_y, 0);
            }
            primary_slot = new_slot;
            primary_fwd_x = -1;
            primary_fwd_y = -1;
        }

        if (primary_slot != -1) {
            int x = last_touch_x[primary_slot];
            int y = last_touch_y[primary_slot];
            if (primary_fwd_x == -1) {
                l_info("[HW_TOUCH_DOWN] Finger at (%d, %d)", x, y);
                if (handleCletEvent) handleCletEvent(&jni, NULL, EVENT_TOUCH_DOWN, x, y, 0);
            } else if (primary_fwd_x != x || primary_fwd_y != y) {
                if (handleCletEvent) handleCletEvent(&jni, NULL, EVENT_TOUCH_MOVE, x, y, 0);
            }
            primary_fwd_x = x;
            primary_fwd_y = y;
        }

        /**< @brief 2. Handle Physical Buttons & Analog Sticks. */
        SceCtrlData pad;
        sceCtrlPeekBufferPositive(0, &pad, 1);

        /**< @brief Directional handling (D-Pad & Left Stick -> Send Walk KeyCodes 50, 56, 52, 54). */
        int up_pressed    = (pad.buttons & SCE_CTRL_UP)    || (pad.ly < 64);
        int down_pressed  = (pad.buttons & SCE_CTRL_DOWN)  || (pad.ly > 192);
        int left_pressed  = (pad.buttons & SCE_CTRL_LEFT)  || (pad.lx < 64);
        int right_pressed = (pad.buttons & SCE_CTRL_RIGHT) || (pad.lx > 192);

        if (up_pressed && !old_up) {
            l_info("[CTRL] DPAD/LSTICK UP DOWN -> Walk Up (Key %d)", KEY_WALK_UP);
            send_key_event(KEY_WALK_UP, 1);
        }
        if (!up_pressed && old_up) {
            send_key_event(KEY_WALK_UP, 0);
        }

        if (down_pressed && !old_down) {
            l_info("[CTRL] DPAD/LSTICK DOWN DOWN -> Walk Down (Key %d)", KEY_WALK_DOWN);
            send_key_event(KEY_WALK_DOWN, 1);
        }
        if (!down_pressed && old_down) {
            send_key_event(KEY_WALK_DOWN, 0);
        }

        if (left_pressed && !old_left) {
            l_info("[CTRL] DPAD/LSTICK LEFT DOWN -> Walk Left (Key %d)", KEY_WALK_LEFT);
            send_key_event(KEY_WALK_LEFT, 1);
        }
        if (!left_pressed && old_left) {
            send_key_event(KEY_WALK_LEFT, 0);
        }

        if (right_pressed && !old_right) {
            l_info("[CTRL] DPAD/LSTICK RIGHT DOWN -> Walk Right (Key %d)", KEY_WALK_RIGHT);
            send_key_event(KEY_WALK_RIGHT, 1);
        }
        if (!right_pressed && old_right) {
            send_key_event(KEY_WALK_RIGHT, 0);
        }

        old_up    = up_pressed;
        old_down  = down_pressed;
        old_left  = left_pressed;
        old_right = right_pressed;

        /**< @brief Cross (X) -> Attack / NPC Talk / Action / Confirm (Key 53). */
        int cross_down = (pad.buttons & SCE_CTRL_CROSS) != 0;
        if (cross_down && !old_cross) {
            l_info("[CTRL] CROSS (X) DOWN -> Attack / Action (Key %d)", KEY_ATTACK);
            send_key_event(KEY_ATTACK, 1);
        }
        if (!cross_down && old_cross) {
            send_key_event(KEY_ATTACK, 0);
        }
        old_cross = cross_down;

        /**< @brief Circle (O) -> Right / Forward Jump in battle (Key -4). */
        int circle_down = (pad.buttons & SCE_CTRL_CIRCLE) != 0;
        if (circle_down && !old_circle) {
            l_info("[CTRL] CIRCLE (O) DOWN -> Right Jump (Key %d)", KEY_JUMP_RIGHT);
            send_key_event(KEY_JUMP_RIGHT, 1);
        }
        if (!circle_down && old_circle) {
            send_key_event(KEY_JUMP_RIGHT, 0);
        }
        old_circle = circle_down;

        /**< @brief Triangle (Δ) -> Left / Backward Jump in battle (Key -3). */
        int triangle_down = (pad.buttons & SCE_CTRL_TRIANGLE) != 0;
        if (triangle_down && !old_triangle) {
            l_info("[CTRL] TRIANGLE (Δ) DOWN -> Left Jump (Key %d)", KEY_JUMP_LEFT);
            send_key_event(KEY_JUMP_LEFT, 1);
        }
        if (!triangle_down && old_triangle) {
            send_key_event(KEY_JUMP_LEFT, 0);
        }
        old_triangle = triangle_down;

        /**< @brief Right Stick: Quick cast skills 1, 2, 3, 4. */
        int rstick_left  = (pad.rx < 64);
        int rstick_up    = (pad.ry < 64);
        int rstick_down  = (pad.ry > 192);
        int rstick_right = (pad.rx > 192);

        /**< @brief Square (□) / Right Stick Left -> Skill 1 (Key -13). */
        int square_down = (pad.buttons & SCE_CTRL_SQUARE) != 0;
        int skill1_down = square_down || rstick_left;
        if (skill1_down && !old_square && !old_rstick_left) {
            l_info("[CTRL] SQUARE / R-STICK LEFT DOWN -> Skill 1 (Key %d)", KEY_SKILL1);
            send_key_event(KEY_SKILL1, 1);
        }
        if (!skill1_down && (old_square || old_rstick_left)) {
            send_key_event(KEY_SKILL1, 0);
        }
        old_square = square_down;
        old_rstick_left = rstick_left;

        if (rstick_up && !old_rstick_up) {
            l_info("[CTRL] R-STICK UP DOWN -> Skill 2 (Key %d)", KEY_SKILL2);
            send_key_event(KEY_SKILL2, 1);
        }
        if (!rstick_up && old_rstick_up) {
            send_key_event(KEY_SKILL2, 0);
        }
        old_rstick_up = rstick_up;

        if (rstick_down && !old_rstick_down) {
            l_info("[CTRL] R-STICK DOWN DOWN -> Skill 3 (Key %d)", KEY_SKILL3);
            send_key_event(KEY_SKILL3, 1);
        }
        if (!rstick_down && old_rstick_down) {
            send_key_event(KEY_SKILL3, 0);
        }
        old_rstick_down = rstick_down;

        if (rstick_right && !old_rstick_right) {
            l_info("[CTRL] R-STICK RIGHT DOWN -> Skill 4 (Key %d)", KEY_SKILL4);
            send_key_event(KEY_SKILL4, 1);
        }
        if (!rstick_right && old_rstick_right) {
            send_key_event(KEY_SKILL4, 0);
        }
        old_rstick_right = rstick_right;

        /**< @brief L1 Trigger -> Teamstrike (T Button) (Key 48). */
        int l1_down = (pad.buttons & SCE_CTRL_L1) != 0;
        if (l1_down && !old_l1) {
            l_info("[CTRL] L1 TRIGGER DOWN -> Teamstrike [T] (Key %d)", KEY_TEAMSTRIKE);
            send_key_event(KEY_TEAMSTRIKE, 1);
        }
        if (!l1_down && old_l1) {
            send_key_event(KEY_TEAMSTRIKE, 0);
        }
        old_l1 = l1_down;

        /**< @brief R1 Trigger -> Character Tag / Swap (Key -12). */
        int r1_down = (pad.buttons & SCE_CTRL_R1) != 0;
        if (r1_down && !old_r1) {
            l_info("[CTRL] R1 TRIGGER DOWN -> Character Tag (Key %d)", KEY_CHAR_SWITCH);
            send_key_event(KEY_CHAR_SWITCH, 1);
        }
        if (!r1_down && old_r1) {
            send_key_event(KEY_CHAR_SWITCH, 0);
        }
        old_r1 = r1_down;

        /**< @brief Select -> Cancel / Back / Inventory in menus (-16). */
        int select_down = (pad.buttons & SCE_CTRL_SELECT) != 0;
        if (select_down && !old_select) {
            l_info("[CTRL] SELECT DOWN -> Back / Menu (Key %d)", KEY_MENU_BACK);
            send_key_event(KEY_MENU_BACK, 1);
        }
        if (!select_down && old_select) {
            send_key_event(KEY_MENU_BACK, 0);
        }
        old_select = select_down;

        // Start -> Pause Menu (Key -8)
        int start_down = (pad.buttons & SCE_CTRL_START) != 0;
        if (start_down && !old_start) {
            l_info("[CTRL] START DOWN -> Pause (Key %d)", KEY_MENU_PAUSE);
            send_key_event(KEY_MENU_PAUSE, 1);
        }
        if (!start_down && old_start) {
            send_key_event(KEY_MENU_PAUSE, 0);
        }
        old_start = start_down;

        // 3. Render Frame
        if (NativeRender) {
            NativeRender(&jni, NULL);
        }

        gl_swap();
        /**
         * @brief No explicit sceDisplayWaitVblankStartMulti() here.
         * @note See `docs/source/main.md:543` for detailed design rationale.
         */
#ifdef INSTRUMENT_GL_CALLS
        gl_instrument_frame_end();
#endif
    }

    sceKernelExitDeleteThread(0);
    return 0;
}
