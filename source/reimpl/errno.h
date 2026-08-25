/**
 * @brief Copyright (C) 2023-2024 Volodymyr Atamanenko This software may be modified and distributed under the terms of the MIT license.
 * @note See `docs/source/reimpl/errno.md:1` for detailed design rationale.
 */

/**
 * @brief Copyright (C) 2023-2024 Volodymyr Atamanenko This software may be modified and distributed under the terms of the MIT license.
 */

#ifndef SOLOADER_ERRNO_H
#define SOLOADER_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

int *__errno_soloader(void);

char *strerror_soloader(int error_number);

int strerror_r_soloader(int error_number, char *buf, size_t buf_len);

#ifdef __cplusplus
};
#endif

#endif // SOLOADER_ERRNO_H
