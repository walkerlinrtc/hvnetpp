#ifndef _RTCLOG_H_
#define _RTCLOG_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <string>
#include "easylogging++.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Log levels matching the project style
enum RtcLogLevel {
    RTC_FATAL = 0,
    RTC_ERROR = 1,
    RTC_WARN  = 2,
    RTC_INFO  = 3,
    RTC_DEBUG = 4,
    RTC_TRACE = 5
};

// Check if a specific log level is enabled
bool rtclog_is_enabled(enum RtcLogLevel lvl);

// Main logging macro consistent with LLOG style
// Optimized to check level before formatting
#define RTCLOG(lvl, fmt, ...)                                          \
    do {                                                               \
        if (rtclog_is_enabled(lvl)) {                                  \
            rtclog_fmt(__FILE__, __LINE__, __FUNCTION__, lvl, fmt, ##__VA_ARGS__); \
        }                                                              \
    } while (0)

// Format function declaration
void rtclog_fmt(const char *filename, int fileline, const char* funcname, enum RtcLogLevel lvl,
              const char *fmt, ...) 
#ifdef __GNUC__
__attribute__((format(printf, 5, 6)))
#endif
;

// Initialization and configuration
void rtclog_init(const char *name);
void rtclog_set_level(enum RtcLogLevel lvl);
void rtclog_set_thread_name(const char *name);

// Assertion Helpers (Ported from el_logger style)
#define RTC_ASSERT(expr)                                                       \
    do {                                                                       \
        if (!(expr)) {                                                         \
            RTCLOG(RTC_FATAL, "Assertion failed: %s", #expr);                  \
        }                                                                      \
    } while (0)

#define RTC_CHECK_AND_RET(expr, ret)                                           \
    do {                                                                       \
        if (!(expr)) {                                                         \
            return ret;                                                        \
        }                                                                      \
    } while (0)

#define RTC_CHECK_AND_RET_LOG(expr, ret, fmt, ...)                             \
    do {                                                                       \
        if (!(expr)) {                                                         \
            RTCLOG(RTC_ERROR, fmt, ##__VA_ARGS__);                             \
            return ret;                                                        \
        }                                                                      \
    } while (0)

#ifdef __cplusplus
} // extern "C"

// C++ specific configuration helpers
void rtclog_configure(const std::string &filename_base = "", bool console = false,
                      const std::size_t max_log_file_size = 524288000, // 500 MB
                      const std::size_t max_log_files = 10);

#endif

#endif // _RTCLOG_H_
