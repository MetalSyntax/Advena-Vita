/*
 * main.c — Main entry point and game loop for Advena PS Vita port
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

// The process's main thread (the one the .so actually runs game logic and
// UI construction on -- shows up in crash dumps as thread "ADVENA001",
// named after the app's own VITA_TITLEID) is created by the SDK's own
// startup code BEFORE main() runs, using whatever size this global says --
// NOT the pthread_create_soloader() path in reimpl/pthr.c (that only
// covers threads the .so spawns itself via pthread_create). Left
// undeclared, it defaults to a small stack (256KB) that is nowhere near
// enough for this engine's deep init/UI call chains (e.g.
// GVUIPlayerController construction -> InitialPlayerPadSet), causing a
// Data Abort stack overflow confirmed via psp2dmp (fault on a plain
// stack-store instruction in a function's own prologue).
unsigned int sceUserMainThreadStackSize = 4 * 1024 * 1024;

// Advena native logical UI/canvas resolution: 400x240 (scales to Vita 960x544)
#define GAME_W 400
#define GAME_H 240
#define SCREEN_W 960
#define SCREEN_H 544

#define ENGINE_LOGICAL_W 400
#define ENGINE_LOGICAL_H 240
#define EVENT_KEY_DOWN   2
#define EVENT_KEY_UP     3
#define EVENT_TOUCH_DOWN 23
#define EVENT_TOUCH_UP   24
#define EVENT_TOUCH_MOVE 25

// Directional Movement KeyCodes (Official Nexus2 / Advena Hal KeyCodes)
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

/*
 * Virtual Touch Hotspots (in 400x240 native design coordinates)
 */
#define DPAD_CENTER_X     60
#define DPAD_CENTER_Y     175
#define DPAD_RADIUS       40

// Left Teamstrike / Quest / Target / "!" button (Button 0 in GVUIPlayerController)
#define BTN_TARGET_X      24
#define BTN_TARGET_Y      96

// Right Character Switch Cycle button (Button 1 in GVUIPlayerController)
#define BTN_CHAR_SWITCH_X 357
#define BTN_CHAR_SWITCH_Y 127

// Battle Action Controls
#define BTN_ATTACK_X      345
#define BTN_ATTACK_Y      185

#define BTN_JUMP_LEFT_X   305
#define BTN_JUMP_LEFT_Y   135

#define BTN_JUMP_RIGHT_X  345
#define BTN_JUMP_RIGHT_Y  115

// Bottom Skill Bar (4 slots)
#define BTN_SKILL1_X      180
#define BTN_SKILL1_Y      215

#define BTN_SKILL2_X      220
#define BTN_SKILL2_Y      215

#define BTN_SKILL3_X      260
#define BTN_SKILL3_Y      215

#define BTN_SKILL4_X      300
#define BTN_SKILL4_Y      215

// Top Bar Elements
#define BTN_PARTY1_X      45
#define BTN_PARTY1_Y      20

#define BTN_BAG_X         200
#define BTN_BAG_Y         20

#define BTN_PAUSE_X       380
#define BTN_PAUSE_Y       20

typedef struct {
    int active;
    int x;
    int y;
    int pointer_id;
    const char *name;
} TouchState;

// Dedicated Pointer IDs for each virtual button (1 to 12)
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

    // Audio initialization
    l_info("Initializing audio subsystem...");
    audio_init();

    // Native initialization (400x240 native UI/logical canvas, 960x544 Vita display).
    // MUST use the real Advena logical canvas (400x240), not GAME_W/GAME_H (480x320):
    // this is what GVUIPlayerController's button-position formulas are anchored
    // against (see ENGINE_LOGICAL_W/H comment above) -- fixes the Teamstrike [T]
    // button (and other UI buttons) rendering off from their intended position.
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

    // Language configuration: English (1)
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

    // Hardware touch slot tracker (max 5 slots)
    #define MAX_TOUCH_SLOTS 5
    int last_touch_x[MAX_TOUCH_SLOTS] = {-1, -1, -1, -1, -1};
    int last_touch_y[MAX_TOUCH_SLOTS] = {-1, -1, -1, -1, -1};
    int slot_hw_id[MAX_TOUCH_SLOTS]   = {-1, -1, -1, -1, -1};

    int old_up = 0, old_down = 0, old_left = 0, old_right = 0;
    int old_cross = 0, old_circle = 0, old_triangle = 0, old_square = 0;
    int old_l1 = 0, old_r1 = 0, old_select = 0, old_start = 0;
    int old_rstick_up = 0, old_rstick_down = 0, old_rstick_left = 0, old_rstick_right = 0;

    SceTouchData touch_data;

    while (1) {
        // 1. Handle Physical Touch Screen Input (with stable slot tracking)
        sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch_data, 1);
        int seen[MAX_TOUCH_SLOTS] = {0};

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
                last_touch_x[slot] = -1;
                last_touch_y[slot] = -1;
            }
            seen[slot] = 1;

            if (last_touch_x[slot] == -1) {
                l_info("[HW_TOUCH_DOWN] Finger at (%d, %d) slot %d", x, y, slot);
                if (handleCletEvent) handleCletEvent(&jni, NULL, EVENT_TOUCH_DOWN, x, y, slot);
            } else if (last_touch_x[slot] != x || last_touch_y[slot] != y) {
                if (handleCletEvent) handleCletEvent(&jni, NULL, EVENT_TOUCH_MOVE, x, y, slot);
            }
            last_touch_x[slot] = x;
            last_touch_y[slot] = y;
        }

        for (int s = 0; s < MAX_TOUCH_SLOTS; s++) {
            if (slot_hw_id[s] != -1 && !seen[s]) {
                l_info("[HW_TOUCH_UP] Finger release at (%d, %d) slot %d", last_touch_x[s], last_touch_y[s], s);
                if (handleCletEvent) handleCletEvent(&jni, NULL, EVENT_TOUCH_UP, last_touch_x[s], last_touch_y[s], s);
                last_touch_x[s] = -1;
                last_touch_y[s] = -1;
                slot_hw_id[s] = -1;
            }
        }

        // 2. Handle Physical Buttons & Analog Sticks
        SceCtrlData pad;
        sceCtrlPeekBufferPositive(0, &pad, 1);

        // Directional handling (D-Pad & Left Stick -> Send Walk KeyCodes 50, 56, 52, 54)
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

        // Virtual D-Pad Touch
        int dpad_dx = 0, dpad_dy = 0;
        if (up_pressed)    dpad_dy -= 1;
        if (down_pressed)  dpad_dy += 1;
        if (left_pressed)  dpad_dx -= 1;
        if (right_pressed) dpad_dx += 1;
        update_virtual_dpad(dpad_dx, dpad_dy);

        // Cross (X) -> Attack / NPC Talk / Action (Key 53)
        int cross_down = (pad.buttons & SCE_CTRL_CROSS) != 0;
        if (cross_down && !old_cross) {
            l_info("[CTRL] CROSS (X) DOWN -> Attack / Action (Key %d)", KEY_ATTACK);
            send_key_event(KEY_ATTACK, 1);
        }
        if (!cross_down && old_cross) {
            send_key_event(KEY_ATTACK, 0);
        }
        old_cross = cross_down;

        // Circle (O) -> Forward / Right Jump in battle (Key -4)
        int circle_down = (pad.buttons & SCE_CTRL_CIRCLE) != 0;
        if (circle_down && !old_circle) {
            l_info("[CTRL] CIRCLE (O) DOWN -> Right Jump (Key %d)", KEY_JUMP_RIGHT);
            send_key_event(KEY_JUMP_RIGHT, 1);
        }
        if (!circle_down && old_circle) {
            send_key_event(KEY_JUMP_RIGHT, 0);
        }
        old_circle = circle_down;

        // Triangle (Δ) -> Backward / Left Jump in battle (Key -3)
        int triangle_down = (pad.buttons & SCE_CTRL_TRIANGLE) != 0;
        if (triangle_down && !old_triangle) {
            l_info("[CTRL] TRIANGLE (Δ) DOWN -> Left Jump (Key %d)", KEY_JUMP_LEFT);
            send_key_event(KEY_JUMP_LEFT, 1);
        }
        if (!triangle_down && old_triangle) {
            send_key_event(KEY_JUMP_LEFT, 0);
        }
        old_triangle = triangle_down;

        // Right Stick: Quick cast skills 1, 2, 3, 4
        int rstick_left  = (pad.rx < 64);
        int rstick_up    = (pad.ry < 64);
        int rstick_down  = (pad.ry > 192);
        int rstick_right = (pad.rx > 192);

        // Square (□) / Right Stick Left -> Skill 1 in battle (Key -13), Submenu in menus (-6)
        int square_down = (pad.buttons & SCE_CTRL_SQUARE) != 0;
        int skill1_down = square_down || rstick_left;
        if (skill1_down && !old_square && !old_rstick_left) {
            l_info("[CTRL] SQUARE / R-STICK LEFT DOWN -> Skill 1 (Key %d, Soft1 %d, Touch %d,%d)", KEY_SKILL1, KEY_MENU_SOFT1, g_btn_skill1.x, g_btn_skill1.y);
            send_key_event(KEY_SKILL1, 1);
            send_key_event(KEY_MENU_SOFT1, 1);
        }
        if (!skill1_down && (old_square || old_rstick_left)) {
            send_key_event(KEY_SKILL1, 0);
            send_key_event(KEY_MENU_SOFT1, 0);
        }
        update_virtual_button(&g_btn_skill1, skill1_down);
        old_square = square_down;
        old_rstick_left = rstick_left;

        if (rstick_up && !old_rstick_up) {
            l_info("[CTRL] R-STICK UP DOWN -> Skill 2 (Key %d, Touch %d,%d)", KEY_SKILL2, g_btn_skill2.x, g_btn_skill2.y);
            send_key_event(KEY_SKILL2, 1);
        }
        if (!rstick_up && old_rstick_up) {
            send_key_event(KEY_SKILL2, 0);
        }
        update_virtual_button(&g_btn_skill2, rstick_up);
        old_rstick_up = rstick_up;

        if (rstick_down && !old_rstick_down) {
            l_info("[CTRL] R-STICK DOWN DOWN -> Skill 3 (Key %d, Touch %d,%d)", KEY_SKILL3, g_btn_skill3.x, g_btn_skill3.y);
            send_key_event(KEY_SKILL3, 1);
        }
        if (!rstick_down && old_rstick_down) {
            send_key_event(KEY_SKILL3, 0);
        }
        update_virtual_button(&g_btn_skill3, rstick_down);
        old_rstick_down = rstick_down;

        if (rstick_right && !old_rstick_right) {
            l_info("[CTRL] R-STICK RIGHT DOWN -> Skill 4 (Key %d, Touch %d,%d)", KEY_SKILL4, g_btn_skill4.x, g_btn_skill4.y);
            send_key_event(KEY_SKILL4, 1);
        }
        if (!rstick_right && old_rstick_right) {
            send_key_event(KEY_SKILL4, 0);
        }
        update_virtual_button(&g_btn_skill4, rstick_right);
        old_rstick_right = rstick_right;

        // L1 Trigger -> Teamstrike (T Button) (Key 48) / Tab (1000)
        int l1_down = (pad.buttons & SCE_CTRL_L1) != 0;
        if (l1_down && !old_l1) {
            l_info("[CTRL] L1 TRIGGER DOWN -> Teamstrike [T] (Key %d, Tab 1000)", KEY_TEAMSTRIKE);
            send_key_event(KEY_TEAMSTRIKE, 1);
            send_key_event(1000, 1);
        }
        if (!l1_down && old_l1) {
            send_key_event(KEY_TEAMSTRIKE, 0);
            send_key_event(1000, 0);
        }
        old_l1 = l1_down;

        // R1 Trigger -> Character Tag / Cycle (Key -12) / Tab (1004)
        int r1_down = (pad.buttons & SCE_CTRL_R1) != 0;
        if (r1_down && !old_r1) {
            l_info("[CTRL] R1 TRIGGER DOWN -> Character Tag (Key %d, Tab 1004)", KEY_CHAR_SWITCH);
            send_key_event(KEY_CHAR_SWITCH, 1);
            send_key_event(1004, 1);
        }
        if (!r1_down && old_r1) {
            send_key_event(KEY_CHAR_SWITCH, 0);
            send_key_event(1004, 0);
        }
        old_r1 = r1_down;

        // Select -> Inventory / Bag (240, 20) & Cancel/Back in menus (-16)
        int select_down = (pad.buttons & SCE_CTRL_SELECT) != 0;
        if (select_down && !old_select) {
            l_info("[CTRL] SELECT DOWN -> Bag / Cancel (Key %d, Touch %d,%d)", KEY_MENU_BACK, g_btn_bag.x, g_btn_bag.y);
            send_key_event(KEY_MENU_BACK, 1);
        }
        if (!select_down && old_select) {
            send_key_event(KEY_MENU_BACK, 0);
        }
        update_virtual_button(&g_btn_bag, select_down);
        old_select = select_down;

        // Start -> Pause (455, 20) & Soft3 Menu key (-8)
        int start_down = (pad.buttons & SCE_CTRL_START) != 0;
        if (start_down && !old_start) {
            l_info("[CTRL] START DOWN -> Pause (Key %d, Touch %d,%d)", KEY_MENU_PAUSE, g_btn_pause.x, g_btn_pause.y);
            send_key_event(KEY_MENU_PAUSE, 1);
        }
        if (!start_down && old_start) {
            send_key_event(KEY_MENU_PAUSE, 0);
        }
        update_virtual_button(&g_btn_pause, start_down);
        old_start = start_down;

        // 3. Render Frame
        if (NativeRender) {
            NativeRender(&jni, NULL);
        }

        gl_swap();
        sceDisplayWaitVblankStartMulti(2); // 30 FPS smooth pacing
    }

    sceKernelExitDeleteThread(0);
    return 0;
}
