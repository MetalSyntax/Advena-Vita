/**
 * @brief Copyright (C) 2021 Andy Nguyen Copyright (C) 2022 Rinnegatamante Copyright (C) 2022-2024 Volodymyr Atamanenko This software may be modified.
 * @note See `docs/source/reimpl/io.md:1` for detailed design rationale.
 */

#include "reimpl/io.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <psp2/kernel/threadmgr.h>

#ifdef USE_SCELIBC_IO
#include <libc_bridge/libc_bridge.h>
#endif

#include "utils/logger.h"
#include "utils/utils.h"

/**< @brief Includes the following inline utilities. */
#include "reimpl/bits/_struct_converters.c"

/**
 * @brief Real save-game / progress files used by the engine (CUISubMenuSaveSlot's s0/s1/s2 slots plus the shared "global" data).
 * @note See `docs/source/reimpl/io.md:34` for detailed design rationale.
 */
static const char *g_save_basenames[] = {
    "s0.dat", "s1.dat", "s2.dat", "g.dat", "g_an_g.dat", NULL
};

static const char *path_basename(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static int is_save_basename(const char *basename) {
    for (int i = 0; g_save_basenames[i]; i++) {
        if (strcmp(basename, g_save_basenames[i]) == 0) return 1;
    }
    return 0;
}

static void resolve_path_soloader(const char *path, char *out, size_t out_len) {
    if (!path || !out) return;

    const char *base = path_basename(path);
    if (is_save_basename(base)) {
        mkdir("ux0:data/advena/saves", 0777);
        snprintf(out, out_len, "ux0:data/advena/saves/%s", base);
        return;
    }

    if (strncmp(path, "ux0:", 4) == 0 || strncmp(path, "app0:", 5) == 0 || strncmp(path, "ur0:", 4) == 0) {
        strncpy(out, path, out_len);
        out[out_len - 1] = '\0';
        return;
    }

    // Skip leading ./
    if (strncmp(path, "./", 2) == 0) {
        path += 2;
    }

    /**
     * @brief Skip leading /sdcard/ or /data/.
     * @note See `docs/source/reimpl/io.md:79` for detailed design rationale.
     */
    if (strncmp(path, "/sdcard/", 8) == 0) {
        path += 8;
    } else if (strncmp(path, "/data/", 6) == 0) {
        path += 6;
    }

    snprintf(out, out_len, "ux0:data/advena/assets/%s", path);
    if (access(out, F_OK) == 0) return;

    snprintf(out, out_len, "ux0:data/advena/%s", path);
    if (access(out, F_OK) == 0) return;

    snprintf(out, out_len, "ux0:data/advena/res/drawable/%s", path);
    if (access(out, F_OK) == 0) return;

    snprintf(out, out_len, "ux0:data/advena/res/raw/%s", path);
    if (access(out, F_OK) == 0) return;

    snprintf(out, out_len, "ux0:data/advena/res/%s", path);
    if (access(out, F_OK) == 0) return;

    snprintf(out, out_len, "ux0:data/advena/saves/%s", path);
    if (access(out, F_OK) == 0) return;

    snprintf(out, out_len, "ux0:data/advena/assets/%s", path);
}

/**
 * @brief Safety net for the "save disappears after a crash" failure mode.
 * @note See `docs/source/reimpl/io.md:107` for detailed design rationale.
 */
static int backup_existing_save_if_present(const char *resolved, char *backup_out, size_t backup_out_len) {
    struct stat st;
    if (stat(resolved, &st) != 0 || st.st_size <= 0) return 0;

    snprintf(backup_out, backup_out_len, "%s.bak", resolved);
    if (rename(resolved, backup_out) == 0) {
        l_info("[IO] Backed up existing save '%s' (%ld bytes) to '%s' before truncating open.",
               resolved, (long)st.st_size, backup_out);
        return 1;
    }

    l_warn("[IO] Failed to back up existing save '%s' before truncating open.", resolved);
    return 0;
}

/**
 * @brief Confirmed on-console (advena_028.log:138-161).
 * @note See `docs/source/reimpl/io.md:134` for detailed design rationale.
 */
#define MAX_TRACKED_SAVE_OPENS 4
typedef struct {
    FILE *fp;
    char path[256];
    char backup[300];
} tracked_save_open_t;
static tracked_save_open_t g_tracked_saves[MAX_TRACKED_SAVE_OPENS];

static void track_save_open(FILE *fp, const char *resolved, const char *backup) {
    for (int i = 0; i < MAX_TRACKED_SAVE_OPENS; i++) {
        if (!g_tracked_saves[i].fp) {
            g_tracked_saves[i].fp = fp;
            snprintf(g_tracked_saves[i].path, sizeof(g_tracked_saves[i].path), "%s", resolved);
            snprintf(g_tracked_saves[i].backup, sizeof(g_tracked_saves[i].backup), "%s", backup);
            return;
        }
    }
    l_warn("[IO] Tracked save-open table full; cannot verify write for '%s'.", resolved);
}

/**
 * @brief Returns the tracked-table index for fp (pre-close bookkeeping), or -1.
 * @note See `docs/source/reimpl/io.md:165` for detailed design rationale.
 */
static int find_tracked_save(FILE *fp) {
    for (int i = 0; i < MAX_TRACKED_SAVE_OPENS; i++) {
        if (g_tracked_saves[i].fp == fp) return i;
    }
    return -1;
}

static void verify_and_recover_save_close(int idx, long pre_close_pos) {
    if (idx < 0) return;

    struct stat st;
    int have_stat = (stat(g_tracked_saves[idx].path, &st) == 0);
    /**
     * @brief Diagnostic: always log what we observed, even when it looks fine, so a future capture can tell "write never happened" (pre_close_pos == 0).
     * @note See `docs/source/reimpl/io.md:178` for detailed design rationale.
     */
    l_info("[IO] Save close check for '%s': ftell()-before-close=%ld bytes, "
           "on-disk size after close=%ld bytes.",
           g_tracked_saves[idx].path, pre_close_pos, have_stat ? (long)st.st_size : -1L);

    if (have_stat && st.st_size == 0) {
        if (rename(g_tracked_saves[idx].backup, g_tracked_saves[idx].path) == 0) {
            l_warn("[IO] Save write to '%s' produced an empty file -- restored previous backup.",
                   g_tracked_saves[idx].path);
        } else {
            l_warn("[IO] Save write to '%s' produced an empty file, and restoring the backup "
                   "'%s' failed too.", g_tracked_saves[idx].path, g_tracked_saves[idx].backup);
        }
    }
    g_tracked_saves[idx].fp = NULL;
}

FILE * fopen_soloader(const char * filename, const char * mode) {
    if (!filename) return NULL;

    if (strcmp(filename, "/proc/cpuinfo") == 0) {
        return fopen_soloader("app0:/cpuinfo", mode);
    } else if (strcmp(filename, "/proc/meminfo") == 0) {
        return fopen_soloader("app0:/meminfo", mode);
    }

    char resolved[256];
    resolve_path_soloader(filename, resolved, sizeof(resolved));

    int made_backup = 0;
    char backup[300];
    if (mode && mode[0] == 'w' && is_save_basename(path_basename(filename))) {
        made_backup = backup_existing_save_if_present(resolved, backup, sizeof(backup));
    }

#ifdef USE_SCELIBC_IO
    FILE* ret = sceLibcBridge_fopen(resolved, mode);
#else
    FILE* ret = fopen(resolved, mode);
#endif

    if (ret)
        l_debug("fopen(%s -> %s, %s): %p", filename, resolved, mode, ret);
    else
        l_warn("fopen(%s -> %s, %s): %p", filename, resolved, mode, ret);

    if (ret && made_backup) {
        track_save_open(ret, resolved, backup);
    }

    return ret;
}

int open_soloader(const char * path, int oflag, ...) {
    if (!path) return -1;

    if (strcmp(path, "/proc/cpuinfo") == 0) {
        return open_soloader("app0:/cpuinfo", oflag);
    } else if (strcmp(path, "/proc/meminfo") == 0) {
        return open_soloader("app0:/meminfo", oflag);
    }

    char resolved[256];
    resolve_path_soloader(path, resolved, sizeof(resolved));

    mode_t mode = 0666;
    if (((oflag & BIONIC_O_CREAT) == BIONIC_O_CREAT) ||
        ((oflag & BIONIC_O_TMPFILE) == BIONIC_O_TMPFILE)) {
        va_list args;
        va_start(args, oflag);
        mode = (mode_t)(va_arg(args, int));
        va_end(args);
    }

    oflag = oflags_bionic_to_newlib(oflag);
    int ret = open(resolved, oflag, mode);
    if (ret >= 0)
        l_debug("open(%s -> %s, %x): %i", path, resolved, oflag, ret);
    else
        l_warn("open(%s -> %s, %x): %i", path, resolved, oflag, ret);
    return ret;
}

int fstat_soloader(int fd, stat64_bionic * buf) {
    struct stat st;
    int res = fstat(fd, &st);

    if (res == 0)
        stat_newlib_to_bionic(&st, buf);

    l_debug("fstat(%i): %i", fd, res);
    return res;
}

int stat_soloader(const char * path, stat64_bionic * buf) {
    if (!path) return -1;

    char resolved[256];
    resolve_path_soloader(path, resolved, sizeof(resolved));

    struct stat st;
    int res = stat(resolved, &st);

    if (res == 0)
        stat_newlib_to_bionic(&st, buf);

    l_debug("stat(%s -> %s): %i", path, resolved, res);
    return res;
}

int access_soloader(const char * path, int mode) {
    if (!path) return -1;

    char resolved[256];
    resolve_path_soloader(path, resolved, sizeof(resolved));

    int res = access(resolved, mode);
    l_debug("access(%s -> %s, %i): %i", path, resolved, mode, res);
    return res;
}

int fclose_soloader(FILE * f) {
    int tracked_idx = find_tracked_save(f);
#ifdef USE_SCELIBC_IO
    long pre_close_pos = (tracked_idx >= 0) ? sceLibcBridge_ftell(f) : -1;
    int ret = sceLibcBridge_fclose(f);
#else
    long pre_close_pos = (tracked_idx >= 0) ? ftell(f) : -1;
    int ret = fclose(f);
#endif

    l_debug("fclose(%p): %i", f, ret);

    verify_and_recover_save_close(tracked_idx, pre_close_pos);

    return ret;
}

int close_soloader(int fd) {
    int ret = close(fd);
    l_debug("close(%i): %i", fd, ret);
    return ret;
}

DIR* opendir_soloader(char* _pathname) {
    char resolved[256];
    resolve_path_soloader(_pathname, resolved, sizeof(resolved));
    DIR* ret = opendir(resolved);
    l_debug("opendir(\"%s\" -> \"%s\"): %p", _pathname, resolved, ret);
    return ret;
}

struct dirent64_bionic * readdir_soloader(DIR * dir) {
    static struct dirent64_bionic dirent_tmp;

    struct dirent* ret = readdir(dir);
    l_debug("readdir(%p): %p", dir, ret);

    if (ret) {
        dirent64_bionic* entry_tmp = dirent_newlib_to_bionic(ret);
        memcpy(&dirent_tmp, entry_tmp, sizeof(dirent64_bionic));
        free(entry_tmp);
        return &dirent_tmp;
    }

    return NULL;
}

int readdir_r_soloader(DIR * dirp, dirent64_bionic * entry,
                       dirent64_bionic ** result) {
    struct dirent dirent_tmp;
    struct dirent * pdirent_tmp;

    int ret = readdir_r(dirp, &dirent_tmp, &pdirent_tmp);

    if (ret == 0) {
        dirent64_bionic* entry_tmp = dirent_newlib_to_bionic(&dirent_tmp);
        memcpy(entry, entry_tmp, sizeof(dirent64_bionic));
        *result = (pdirent_tmp != NULL) ? entry : NULL;
        free(entry_tmp);
    }

    l_debug("readdir_r(%p, %p, %p): %i", dirp, entry, result, ret);
    return ret;
}

int closedir_soloader(DIR * dir) {
    int ret = closedir(dir);
    l_debug("closedir(%p): %i", dir, ret);
    return ret;
}

int fcntl_soloader(int fd, int cmd, ...) {
    l_warn("fcntl(%i, %i, ...): not implemented", fd, cmd);
    return 0;
}

int ioctl_soloader(int fd, int request, ...) {
    l_warn("ioctl(%i, %i, ...): not implemented", fd, request);
    return 0;
}

int fsync_soloader(int fd) {
    int ret = fsync(fd);
    l_debug("fsync(%i): %i", fd, ret);
    return ret;
}
