/**
 * @brief Copyright (C) 2021 Rinnegatamante Copyright (C) 2022-2024 Volodymyr Atamanenko This software may be modified and distributed under the.
 * @note See `docs/source/utils/utils.md:1` for detailed design rationale.
 */

/**
 * @brief Copyright (C) 2021 Rinnegatamante Copyright (C) 2022-2024 Volodymyr Atamanenko This software may be modified and distributed under the.
 */

#ifndef SOLOADER_UTILS_H
#define SOLOADER_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Get Unix timestamp in milliseconds.
 * @note See `docs/source/utils/utils.md:25` for detailed design rationale.
 */
uint64_t current_timestamp_ms();

/**
 * @brief Create a copy of a file.
 * @note See `docs/source/utils/utils.md:32` for detailed design rationale.
 */
bool file_copy(const char * path, const char * destination);

/**
 * @brief Check whether a file exists.
 * @note See `docs/source/utils/utils.md:49` for detailed design rationale.
 */
bool file_exists(const char * path);

/**
 * @brief Load file contents into memory.
 * @note See `docs/source/utils/utils.md:58` for detailed design rationale.
 */
bool file_load(const char * path, uint8_t ** buffer, size_t * size);

/**
 * @brief Create directories leading to file.
 * @note See `docs/source/utils/utils.md:70` for detailed design rationale.
 */
bool file_mkpath(const char * path, mode_t mode);

/**
 * @brief Save buffer contents into a file.
 * @note See `docs/source/utils/utils.md:80` for detailed design rationale.
 */
bool file_save(const char * path, const uint8_t * buffer, size_t size);

/**
 * @brief Get the size of a file in bytes @param[in] path Full path of the target file.
 * @note See `docs/source/utils/utils.md:91` for detailed design rationale.
 */
size_t file_size(const char * path);

/**
 * @brief Get SHA1 hash of file contents.
 * @note See `docs/source/utils/utils.md:100` for detailed design rationale.
 */
char * file_sha1sum(const char * path);

/**
 * @brief Check whether specified path is a directory.
 * @note See `docs/source/utils/utils.md:110` for detailed design rationale.
 */
bool is_dir(const char * path);

/**
 * @brief Check whether system module is loaded.
 * @note See `docs/source/utils/utils.md:119` for detailed design rationale.
 */
bool module_loaded(const char * name);

/**
 * @brief Do nothing, return 0.
 * @note See `docs/source/utils/utils.md:128` for detailed design rationale.
 */
int ret0(void);

/**
 * @brief Do nothing, return 1.
 * @note See `docs/source/utils/utils.md:134` for detailed design rationale.
 */
int ret1(void);

/**
 * @brief Do nothing, return -1.
 * @note See `docs/source/utils/utils.md:140` for detailed design rationale.
 */
int retminus1(void);

/**
 * @brief Replace all occurrences of a substring in a string.
 * @note See `docs/source/utils/utils.md:146` for detailed design rationale.
 */
void str_replace(char ** str, const char * needle, const char * replacement);

/**
 * @brief Remove all occurrences of a substring from a string.
 * @note See `docs/source/utils/utils.md:156` for detailed design rationale.
 */
void str_remove(char * str, const char * needle);

/**
 * @brief Check whether a string starts with a substring.
 * @note See `docs/source/utils/utils.md:164` for detailed design rationale.
 */
bool str_starts_with(const char * str, const char * prefix);

/**
 * @brief Check whether a string ends with a substring.
 * @note See `docs/source/utils/utils.md:174` for detailed design rationale.
 */
bool str_ends_with(const char * str, const char * suffix);

/**
 * @brief Get SHA1 hash of a string or byte array.
 * @note See `docs/source/utils/utils.md:184` for detailed design rationale.
 */
char * str_sha1sum(const char * str, size_t size);

#ifdef __cplusplus
};
#endif

#endif // SOLOADER_UTILS_H
