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

#define GAME_W 400
#define GAME_H 240
#define SCREEN_W 960
#define SCREEN_H 544

#define EVENT_TOUCH_DOWN 23
#define EVENT_TOUCH_UP   24
#define EVENT_TOUCH_MOVE 25

#define HAL_KEY_UP    (-1)
#define HAL_KEY_DOWN  (-2)
#define HAL_KEY_LEFT  (-3)
#define HAL_KEY_RIGHT (-4)
#define HAL_KEY_OK    (-5)
#define HAL_KEY_BACK  (-16)

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
 * Virtual Gamepad Touch Hotspots (in 400x240 coordinates)
 */
#define DPAD_CENTER_X 60
#define DPAD_CENTER_Y 175
#define DPAD_RADIUS   40

#define BTN_ATTACK_X  345
#define BTN_ATTACK_Y  185

#define BTN_SKILL1_X  295
#define BTN_SKILL1_Y  185

#define BTN_SKILL2_X  305
#define BTN_SKILL2_Y  135

#define BTN_SKILL3_X  345
#define BTN_SKILL3_Y  115

#define BTN_PAUSE_X   380
#define BTN_PAUSE_Y   20

typedef struct {
    int active;
    int x;
    int y;
    int pointer_id;
} TouchState;

static TouchState g_dpad_touch = {0, 0, 0, 1};
static TouchState g_btn_attack = {0, BTN_ATTACK_X, BTN_ATTACK_Y, 2};
static TouchState g_btn_skill1 = {0, BTN_SKILL1_X, BTN_SKILL1_Y, 3};
static TouchState g_btn_skill2 = {0, BTN_SKILL2_X, BTN_SKILL2_Y, 4};
static TouchState g_btn_skill3 = {0, BTN_SKILL3_X, BTN_SKILL3_Y, 5};
static TouchState g_btn_pause  = {0, BTN_PAUSE_X, BTN_PAUSE_Y, 6};

static void update_virtual_button(TouchState *btn, int is_down) {
    if (is_down && !btn->active) {
        btn->active = 1;
        if (handleCletEvent) handleCletEvent(&jni, NULL, EVENT_TOUCH_DOWN, btn->x, btn->y, btn->pointer_id);
    } else if (!is_down && btn->active) {
        btn->active = 0;
        if (handleCletEvent) handleCletEvent(&jni, NULL, EVENT_TOUCH_UP, btn->x, btn->y, btn->pointer_id);
    }
}

static void update_virtual_dpad(int dx, int dy) {
    if (dx == 0 && dy == 0) {
        if (g_dpad_touch.active) {
            g_dpad_touch.active = 0;
            if (handleCletEvent) handleCletEvent(&jni, NULL, EVENT_TOUCH_UP, g_dpad_touch.x, g_dpad_touch.y, g_dpad_touch.pointer_id);
        }
        return;
    }

    int tx = DPAD_CENTER_X + dx * DPAD_RADIUS;
    int ty = DPAD_CENTER_Y + dy * DPAD_RADIUS;

    if (!g_dpad_touch.active) {
        g_dpad_touch.active = 1;
        g_dpad_touch.x = tx;
        g_dpad_touch.y = ty;
        if (handleCletEvent) handleCletEvent(&jni, NULL, EVENT_TOUCH_DOWN, tx, ty, g_dpad_touch.pointer_id);
    } else if (g_dpad_touch.x != tx || g_dpad_touch.y != ty) {
        g_dpad_touch.x = tx;
        g_dpad_touch.y = ty;
        if (handleCletEvent) handleCletEvent(&jni, NULL, EVENT_TOUCH_MOVE, tx, ty, g_dpad_touch.pointer_id);
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

    if (Game_JNI_OnLoad) {
        l_info("Calling JNI_OnLoad...");
        Game_JNI_OnLoad(&jvm, NULL);
    }

    if (InitializeJNIGlobalRef) {
        l_info("Calling InitializeJNIGlobalRef...");
        InitializeJNIGlobalRef(&jni, NULL);
    }

    l_info("Initializing audio subsystem...");
    audio_init();

    l_info("Initializing OpenGL...");
    gl_init();

    if (NativeInitWithBufferSize) {
        l_info("Calling NativeInitWithBufferSize(%d, %d)...", GAME_W, GAME_H);
        NativeInitWithBufferSize(&jni, NULL, GAME_W, GAME_H);
    }

    if (NativeInitDeviceInfo) {
        l_info("Calling NativeInitDeviceInfo(%d, %d)...", GAME_W, GAME_H);
        NativeInitDeviceInfo(&jni, NULL, GAME_W, GAME_H);
    }

    if (NativeResize) {
        l_info("Calling NativeResize(%d, %d)...", SCREEN_W, SCREEN_H);
        NativeResize(&jni, NULL, SCREEN_W, SCREEN_H);
    }

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);

    l_success("Advena loaded successfully. Entering main render loop.");

    SceTouchData touch_old, touch_new;
    memset(&touch_old, 0, sizeof(touch_old));

    while (1) {
        // Handle Touch Screen Input
        sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch_new, 1);
        for (int i = 0; i < touch_new.reportNum; i++) {
            int x = (touch_new.report[i].x * GAME_W) / 1920;
            int y = (touch_new.report[i].y * GAME_H) / 1088;
            int id = touch_new.report[i].id;

            int found = 0;
            for (int j = 0; j < touch_old.reportNum; j++) {
                if (touch_old.report[j].id == id) {
                    found = 1;
                    break;
                }
            }

            if (!found) {
                if (handleCletEvent) handleCletEvent(&jni, NULL, EVENT_TOUCH_DOWN, x, y, id);
            } else {
                if (handleCletEvent) handleCletEvent(&jni, NULL, EVENT_TOUCH_MOVE, x, y, id);
            }
        }

        for (int i = 0; i < touch_old.reportNum; i++) {
            int id = touch_old.report[i].id;
            int found = 0;
            for (int j = 0; j < touch_new.reportNum; j++) {
                if (touch_new.report[j].id == id) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                int x = (touch_old.report[i].x * GAME_W) / 1920;
                int y = (touch_old.report[i].y * GAME_H) / 1088;
                if (handleCletEvent) handleCletEvent(&jni, NULL, EVENT_TOUCH_UP, x, y, id);
            }
        }
        memcpy(&touch_old, &touch_new, sizeof(touch_new));

        // Handle Physical Buttons & Analog Sticks
        SceCtrlData pad;
        sceCtrlPeekBufferPositive(0, &pad, 1);

        int dpad_dx = 0, dpad_dy = 0;
        if (pad.buttons & SCE_CTRL_UP || pad.ly < 64) dpad_dy -= 1;
        if (pad.buttons & SCE_CTRL_DOWN || pad.ly > 192) dpad_dy += 1;
        if (pad.buttons & SCE_CTRL_LEFT || pad.lx < 64) dpad_dx -= 1;
        if (pad.buttons & SCE_CTRL_RIGHT || pad.lx > 192) dpad_dx += 1;

        update_virtual_dpad(dpad_dx, dpad_dy);

        update_virtual_button(&g_btn_attack, (pad.buttons & SCE_CTRL_CROSS) != 0);
        update_virtual_button(&g_btn_skill1, (pad.buttons & SCE_CTRL_SQUARE) != 0);
        update_virtual_button(&g_btn_skill2, (pad.buttons & SCE_CTRL_TRIANGLE) != 0);
        update_virtual_button(&g_btn_skill3, (pad.buttons & SCE_CTRL_CIRCLE) != 0);
        update_virtual_button(&g_btn_pause,  (pad.buttons & SCE_CTRL_START) != 0);

        // Render Frame
        if (NativeRender) {
            NativeRender(&jni, NULL);
        }

        gl_swap();
        sceDisplayWaitVblankStartMulti(2); // 30 FPS smooth pacing
    }

    sceKernelExitDeleteThread(0);
    return 0;
}
