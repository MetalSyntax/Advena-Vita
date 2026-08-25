/**
 * @brief Debugging TIME_64_DEBUG Define if you want debugging messages.
 * @note See `docs/source/reimpl/time64_config.md:1` for detailed design rationale.
 */
/**
 * @brief define TIME_64_DEBUG.
 * @note See `docs/source/reimpl/time64_config.md:5` for detailed design rationale.
 */


/**
 * @brief INT_64_T A 64 bit integer type to use to store time and others.
 * @note See `docs/source/reimpl/time64_config.md:8` for detailed design rationale.
 */
#define INT_64_T                long long


/**
 * @brief USE_TM64 Should we use a 64 bit safe replacement for tm.
 * @note See `docs/source/reimpl/time64_config.md:15` for detailed design rationale.
 */
/**
 * @brief define USE_TM64.
 * @note See `docs/source/reimpl/time64_config.md:20` for detailed design rationale.
 */


/**
 * @brief Availability of system functions.
 * @note See `docs/source/reimpl/time64_config.md:23` for detailed design rationale.
 */
#define HAS_GMTIME_R
#define HAS_LOCALTIME_R
#define HAS_TIMEGM


/**
 * @brief Details of non-standard tm struct elements.
 * @note See `docs/source/reimpl/time64_config.md:39` for detailed design rationale.
 */
/**
 * @brief define HAS_TM_TM_GMTOFF #define HAS_TM_TM_ZONE.
 * @note See `docs/source/reimpl/time64_config.md:49` for detailed design rationale.
 */


/**
 * @brief USE_SYSTEM_LOCALTIME USE_SYSTEM_GMTIME Should we use the system functions if the time is inside their range.
 * @note See `docs/source/reimpl/time64_config.md:53` for detailed design rationale.
 */
#define USE_SYSTEM_LOCALTIME
/**
 * @brief define USE_SYSTEM_GMTIME.
 * @note See `docs/source/reimpl/time64_config.md:60` for detailed design rationale.
 */


/**
 * @brief SYSTEM_LOCALTIME_MAX SYSTEM_LOCALTIME_MIN SYSTEM_GMTIME_MAX SYSTEM_GMTIME_MIN Maximum and minimum values your system's gmtime() and.
 * @note See `docs/source/reimpl/time64_config.md:63` for detailed design rationale.
 */
#define SYSTEM_LOCALTIME_MAX     2147483647
#define SYSTEM_LOCALTIME_MIN    -2147483647
#define SYSTEM_GMTIME_MAX        2147483647
#define SYSTEM_GMTIME_MIN       -2147483647

