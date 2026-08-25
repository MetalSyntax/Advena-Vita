/**
 * @brief Copyright (C) 2009 The Android Open Source Project Copyright (C) 2021 Andy Nguyen Copyright (C) 2022 Rinnegatamante Copyright (C) 2022-2023.
 * @note See `docs/source/reimpl/log.md:1` for detailed design rationale.
 */

/**
 * @brief Copyright (C) 2021 Andy Nguyen Copyright (C) 2022 Rinnegatamante Copyright (C) 2022-2024 Volodymyr Atamanenko This software may be modified.
 */

#ifndef SOLOADER_LOG_H
#define SOLOADER_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

/**
 * @brief Android log priority values, in increasing order of priority.
 * @note See `docs/source/reimpl/log.md:25` for detailed design rationale.
 */
typedef enum android_LogPriority {
    /**
     * @brief For internal use only.
     * @note See `docs/source/reimpl/log.md:29` for detailed design rationale.
     */
    ANDROID_LOG_UNKNOWN = 0,
    /**
     * @brief The default priority, for internal use only.
     * @note See `docs/source/reimpl/log.md:31` for detailed design rationale.
     */
    ANDROID_LOG_DEFAULT, /* only for SetMinPriority() */
    /**
     * @brief Verbose logging.
     * @note See `docs/source/reimpl/log.md:33` for detailed design rationale.
     */
    ANDROID_LOG_VERBOSE,
    /**
     * @brief Debug logging. Should typically be disabled for a release apk.
     * @note See `docs/source/reimpl/log.md:35` for detailed design rationale.
     */
    ANDROID_LOG_DEBUG,
    /**
     * @brief Informational logging.
     * @note See `docs/source/reimpl/log.md:37` for detailed design rationale.
     */
    ANDROID_LOG_INFO,
    /**
     * @brief Warning logging.
     * @note See `docs/source/reimpl/log.md:39` for detailed design rationale.
     */
    ANDROID_LOG_WARN,
    /**
     * @brief Error logging. For use with unrecoverable failures.
     * @note See `docs/source/reimpl/log.md:41` for detailed design rationale.
     */
    ANDROID_LOG_ERROR,
    /**
     * @brief Fatal logging. For use when aborting.
     * @note See `docs/source/reimpl/log.md:43` for detailed design rationale.
     */
    ANDROID_LOG_FATAL,
    /**
     * @brief For internal use only.
     * @note See `docs/source/reimpl/log.md:45` for detailed design rationale.
     */
    ANDROID_LOG_SILENT, /* only for SetMinPriority(); must be last */
} android_LogPriority;

/**
 * @brief Writes the constant string `text` to the log, with priority `prio` and tag `tag`.
 * @note See `docs/source/reimpl/log.md:49` for detailed design rationale.
 */
int __android_log_write(int prio, const char *tag, const char *text);

/**
 * @brief Writes a formatted string to the log, with priority `prio` and tag `tag`.
 * @note See `docs/source/reimpl/log.md:55` for detailed design rationale.
 */
int __android_log_print(int prio, const char *tag, const char *fmt, ...)
    __attribute__((__format__(printf, 3, 4)));

/**
 * @brief Equivalent to `__android_log_print`, but taking a `va_list`.
 * @note See `docs/source/reimpl/log.md:63` for detailed design rationale.
 */
int __android_log_vprint(int prio, const char *tag, const char *fmt, va_list ap)
    __attribute__((__format__(printf, 3, 0)));

/**
 * @brief Writes an assertion failure to the log (as `ANDROID_LOG_FATAL`) and to stderr, before calling.
 * @note See `docs/source/reimpl/log.md:70` for detailed design rationale.
 */
void __android_log_assert(const char* cond, const char* tag, const char* fmt, ...)
__attribute__((__noreturn__)) __attribute__((__format__(printf, 3, 4)));

#ifdef __cplusplus
};
#endif

#endif // SOLOADER_LOG_H
