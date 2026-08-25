/**
 * @brief Copyright (C) 2021 Andy Nguyen Copyright (C) 2022 Rinnegatamante Copyright (C) 2022-2023 Volodymyr Atamanenko This software may be modified.
 * @note See `docs/source/reimpl/mem.md:1` for detailed design rationale.
 */

#include "reimpl/mem.h"
#include "utils/logger.h"

#include <string.h>
#include <malloc.h>
#include <psp2/kernel/clib.h>

void *sceClibMemclr(void *dst, size_t len) {
    return sceClibMemset(dst, 0, len);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offs) {
    l_warn("mmap(%p, %i, %i, %i, %i, %li)", addr, length, prot, flags, fd, offs);

    if (length <= 0) {
        return MAP_FAILED;
    }
    void* ret= malloc(length);
    memset(ret, 0, length);
    return ret;
}

int munmap(void *addr, size_t length) {
    if (addr) free(addr);
    return 0;
}
