/**
 * @brief Copyright (C) 2022-2024 Volodymyr Atamanenko This software may be modified and distributed under the terms of the MIT license.
 * @note See `docs/source/utils/logger.md:1` for detailed design rationale.
 */

#include "utils/logger.h"

#include <psp2/kernel/clib.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/io/stat.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdatomic.h>

#define COLOR_RED    "\x1B[38;5;196m"
#define COLOR_PINK   "\x1B[38;5;212m"
#define COLOR_ORANGE "\x1B[38;5;202m"
#define COLOR_BLUE   "\x1B[38;5;32m"
#define COLOR_GREEN  "\x1B[32m"
#define COLOR_CYAN   "\x1B[36m"

#define COLOR_END    "\033[0m"

static SceKernelLwMutexWork _log_mutex;
static atomic_bool _log_mutex_ready = ATOMIC_VAR_INIT(false);
static FILE *_log_file = NULL;
static char _current_log_filename[128] = "advena_001.log";

static void _ensure_log_file(void) {
    if (!_log_file) {
        sceIoMkdir("ux0:data", 0777);
        sceIoMkdir("ux0:data/advena", 0777);
        sceIoMkdir("ux0:data/advena/logs", 0777);

        int chosen_index = 1;
        char filepath[256];

        /**
         * @brief Search for the next available log file from 001 to 999.
         * @note See `docs/source/utils/logger.md:41` for detailed design rationale.
         */
        for (int i = 1; i <= 999; i++) {
            snprintf(filepath, sizeof(filepath), "ux0:data/advena/logs/advena_%03d.log", i);
            FILE *test = fopen(filepath, "r");
            if (test) {
                fclose(test);
            } else {
                chosen_index = i;
                break;
            }
        }

        snprintf(_current_log_filename, sizeof(_current_log_filename), "advena_%03d.log", chosen_index);
        snprintf(filepath, sizeof(filepath), "ux0:data/advena/logs/%s", _current_log_filename);

        _log_file = fopen(filepath, "w");

        if (_log_file) {
            fprintf(_log_file, "=== Advena (PS Vita) Log Started [%s] ===\n", _current_log_filename);
            fflush(_log_file);
        }
    }
}

static char buffer_a[2048];
static char buffer_b[2048];
static char buffer_file[2048];

void _log_print(int t, const char* fmt, ...) {
    if (!atomic_load_explicit(&_log_mutex_ready, memory_order_relaxed)) {
        int ret = sceKernelCreateLwMutex(&_log_mutex, "log_lock", 0, 0, NULL);
        if (ret < 0) {
            sceClibPrintf("Error: failed to create log mutex: 0x%x\n", ret);
            return;
        }
        atomic_store_explicit(&_log_mutex_ready, true, memory_order_relaxed);
    }
    sceKernelLockLwMutex(&_log_mutex, 1, NULL);

    _ensure_log_file();

    const char *tag = "info";
    switch (t) {
        case LT_DEBUG:
            tag = "debug";
            sceClibSnprintf(buffer_a, sizeof(buffer_a), " %s• debug%s    %s\n",
                            COLOR_PINK, COLOR_END, fmt); break;
        case LT_INFO:
            tag = "info";
            sceClibSnprintf(buffer_a, sizeof(buffer_a), " %sℹ info%s     %s\n",
                            COLOR_BLUE, COLOR_END, fmt); break;
        case LT_WARN:
            tag = "warn";
            sceClibSnprintf(buffer_a, sizeof(buffer_a), " %s⚠ warning%s  %s\n",
                            COLOR_ORANGE, COLOR_END, fmt); break;
        case LT_ERROR:
            tag = "error";
            sceClibSnprintf(buffer_a, sizeof(buffer_a), " %s⨯ error%s    %s\n",
                            COLOR_RED, COLOR_END, fmt); break;
        case LT_FATAL:
            tag = "fatal";
            sceClibSnprintf(buffer_a, sizeof(buffer_a), " %s! fatal%s    %s\n",
                            COLOR_RED, COLOR_END, fmt); break;
        case LT_SUCCESS:
            tag = "success";
            sceClibSnprintf(buffer_a, sizeof(buffer_a), " %s! success%s  %s\n",
                            COLOR_GREEN, COLOR_END, fmt); break;
        case LT_WAIT:
            tag = "wait";
            sceClibSnprintf(buffer_a, sizeof(buffer_a), " %s… waiting%s  %s\n",
                            COLOR_CYAN, COLOR_END, fmt); break;
        default:
            if (atomic_load_explicit(&_log_mutex_ready, memory_order_relaxed)) {
                sceKernelUnlockLwMutex(&_log_mutex, 1);
            }
            return;
    }

    va_list list;
    va_start(list, fmt);
    sceClibVsnprintf(buffer_b, sizeof(buffer_b), buffer_a, list);
    va_end(list);

    sceClibPrintf("%s", buffer_b);

    if (_log_file) {
        va_list list_file;
        va_start(list_file, fmt);
        vsnprintf(buffer_file, sizeof(buffer_file), fmt, list_file);
        va_end(list_file);

        fprintf(_log_file, "[%s] %s\n", tag, buffer_file);
        fflush(_log_file);
    }

    if (atomic_load_explicit(&_log_mutex_ready, memory_order_relaxed)) {
        sceKernelUnlockLwMutex(&_log_mutex, 1);
    }
}

void game_log(const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    char buf[1024];
    sceClibVsnprintf(buf, sizeof(buf), fmt, list);
    va_end(list);
    sceClibPrintf("%s", buf);

    _ensure_log_file();
    if (_log_file) {
        fprintf(_log_file, "%s", buf);
        fflush(_log_file);
    }
}

