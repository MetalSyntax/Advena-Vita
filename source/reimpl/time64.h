/**
 * @brief Copyright (c) 2007-2008 Michael G Schwern This software originally derived from Paul Sheer's pivotal_gmtime_r.
 * @note See `docs/source/reimpl/time64.md:1` for detailed design rationale.
 */
#ifndef TIME64_H
#define TIME64_H

#if defined(__LP64__)

#error Your time_t is already 64-bit.

#else

/**
 * @brief Legacy cruft for LP32 where time_t was 32-bit.
 * @note See `docs/source/reimpl/time64.md:37` for detailed design rationale.
 */

#include <sys/cdefs.h>
#include <time.h>
#include <stdint.h>

__BEGIN_DECLS

typedef int64_t time64_t;

char* _Nullable asctime64(const struct tm* _Nonnull);
char* _Nullable asctime64_r(const struct tm* _Nonnull, char* _Nonnull);
char* _Nullable ctime64(const time64_t* _Nonnull);
char* _Nullable ctime64_r(const time64_t* _Nonnull, char* _Nonnull);
struct tm* _Nullable gmtime64(const time64_t* _Nonnull);
struct tm* _Nullable gmtime64_r(const time64_t* _Nonnull, struct tm* _Nonnull);
struct tm* _Nullable localtime64(const time64_t* _Nonnull);
struct tm* _Nullable localtime64_r(const time64_t* _Nonnull, struct tm* _Nonnull);
time64_t mktime64(const struct tm* _Nonnull);
time64_t timegm64(const struct tm* _Nonnull);
time64_t timelocal64(const struct tm* _Nonnull);

__END_DECLS

#endif

#endif /* TIME64_H */
