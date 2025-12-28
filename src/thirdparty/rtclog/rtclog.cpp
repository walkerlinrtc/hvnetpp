#include "rtclog.h"

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <atomic>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <stdarg.h>

#undef RTC_DEFAULT_LOG_CATEGORY
#define RTC_DEFAULT_LOG_CATEGORY "RTCLOG"

#define RTCLOG_BASE_FORMAT "%datetime{%Y-%M-%d %H:%m:%s.%g} %level %logger %fbase:%line %msg"

using namespace std;

// Helper functions
static std::vector<std::string>
vid_string_split(std::string str, std::string flag)
{
    std::vector<std::string> arr;
    if (flag.empty()) {
        arr.push_back(str);
        return arr;
    }
    size_t pos;
    std::string s = str;
    while ((pos = s.find(flag)) != std::string::npos) {
        if (pos != 0) {
            arr.push_back(s.substr(0, pos));
        }
        s = s.substr(pos + flag.length());
    }
    if (!s.empty()) {
        arr.push_back(s);
    }
    return arr;
}

extern "C"
{
#include <unistd.h>
}

std::string
rtclog_get_default_log_path(const char *default_filename)
{
    std::string log_path;
    char *buffer;
    if ((buffer = getcwd(NULL, 0)) != NULL) {
        log_path.append(buffer);
        log_path.append("/");
        free(buffer);
    }
    log_path.append("logs/");
    log_path.append(default_filename);
    return log_path;
}

static void rtclog_set_common_prefix() {}

static const char *
get_default_categories(int level)
{
    const char *categories = "";
    switch (level) {
        case 0:
        case 1:
            categories = "*:INFO,rtclog:INFO,global:INFO";
            break;
        case 2:
            categories = "*:DEBUG";
            break;
        case 3:
            categories = "*:TRACE";
            break;
        case 4:
            categories = "*:TRACE";
            break;
        default:
            break;
    }
    return categories;
}

// Forward declaration
void rtclog_set_log(const char *log);

void
rtclog_configure(const std::string &filename_base, bool console, const std::size_t max_log_file_size,
               const std::size_t max_log_files)
{
    el::Configurations c;
    std::string _filename_base;

    if (filename_base.empty()) {
        _filename_base = rtclog_get_default_log_path("rtclog.log");
    } else if (filename_base.find("/") == std::string::npos) {
        _filename_base = rtclog_get_default_log_path(filename_base.c_str());
    } else {
        _filename_base = filename_base;
    }

    c.setGlobally(el::ConfigurationType::Filename, _filename_base);
    c.setGlobally(el::ConfigurationType::ToFile, "true");
    const char *log_format = getenv("RTC_LOG_FORMAT");
    if (!log_format)
        log_format = RTCLOG_BASE_FORMAT;
    c.setGlobally(el::ConfigurationType::Format, log_format);
    c.setGlobally(el::ConfigurationType::ToStandardOutput, console ? "true" : "false");
    c.setGlobally(el::ConfigurationType::MaxLogFileSize, std::to_string(max_log_file_size));
    
    el::Loggers::setDefaultConfigurations(c, true);

    // Also explicitly configure the rtclog category logger if it exists
    el::Logger* rtclogLogger = el::Loggers::getLogger(RTC_DEFAULT_LOG_CATEGORY);
    if (rtclogLogger) {
        rtclogLogger->configure(c);
    }

    el::Loggers::addFlag(el::LoggingFlag::AutoSpacing);
    el::Loggers::addFlag(el::LoggingFlag::HierarchicalLogging);
    el::Loggers::addFlag(el::LoggingFlag::NewLineForContainer);
    el::Loggers::addFlag(el::LoggingFlag::CreateLoggerAutomatically);
    el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);
    el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
    el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);

    el::Helpers::installPreRollOutCallback([_filename_base, max_log_files](const char *name, size_t size) {
        char abs_filename[PATH_MAX];
        std::string abs_file_path, rname;
        if (realpath(name, abs_filename)) {
            abs_file_path = std::string(abs_filename);
            const size_t last_slash_idx = abs_file_path.find_last_of("/");
            if (std::string::npos != last_slash_idx) {
                abs_file_path.erase(last_slash_idx + 1, -1);
            }
        } else {
            return;
        }

        // BACK IT UP
        std::stringstream ss;
        auto time_now = chrono::system_clock::now();
        auto duration_in_ms = chrono::duration_cast<chrono::milliseconds>(time_now.time_since_epoch());

        rname = abs_file_path + "rtclog-backup-" + std::to_string(duration_in_ms.count()) + ".log";

        int ret = rename(name, rname.c_str());
        if (ret < 0) {
            return;
        }

        if (max_log_files != 0) {
            std::vector<std::string> found_files;
            const char *dir_name = abs_file_path.c_str();
            struct stat s;
            stat(dir_name, &s);
            if (!S_ISDIR(s.st_mode)) {
                return;
            }

            struct dirent *fname;
            DIR *dir;

            dir = opendir(dir_name);
            if (NULL == dir) {
                return;
            }
            while ((fname = readdir(dir)) != NULL) {
                if (strcmp(fname->d_name, ".") == 0 || strcmp(fname->d_name, "..") == 0) {
                    continue;
                }

                if (fname->d_type != DT_REG) {
                    continue;
                }

                if (strncmp(fname->d_name, "rtclog-backup-", sizeof("rtclog-backup-") - 1) != 0) {
                    continue;
                }

                found_files.push_back(fname->d_name);
            }

            closedir(dir);

            if (found_files.size() >= max_log_files) {
                std::sort(found_files.begin(), found_files.end(), [](const std::string &fa, const std::string &fb) {
                    struct stat sa, sb;
                    if (stat(fa.c_str(), &sa) == 0 && stat(fb.c_str(), &sb) == 0) {
                        return sa.st_mtime < sb.st_mtime;
                    } else {
                        return strcmp(fa.c_str(), fb.c_str()) < 0;
                    }
                });

                for (size_t i = 0; i <= found_files.size() - max_log_files; ++i) {
                    std::string fname = abs_file_path + found_files[i];
                    remove(fname.c_str());
                }
            }
        }
    });
    rtclog_set_common_prefix();
    const char *rtc_log = getenv("RTC_LOGS");
    if (!rtc_log) {
        rtc_log = get_default_categories(0);
    }
    rtclog_set_log(rtc_log);
}

void
rtclog_set_categories(const char *categories)
{
    std::string new_categories;
    if (*categories) {
        if (*categories == '+') {
            ++categories;
            new_categories = el::Loggers::getCategories();
            if (*categories) {
                if (!new_categories.empty())
                    new_categories += ",";
                new_categories += categories;
            }
        } else if (*categories == '-') {
            ++categories;
            new_categories = el::Loggers::getCategories();
            std::vector<std::string> single_categories;
            single_categories = vid_string_split(categories, ",");

            for (const std::string &s : single_categories) {
                size_t pos = new_categories.find(s);
                if (pos != std::string::npos)
                    new_categories = new_categories.erase(pos, s.size());
            }
        } else {
            new_categories = categories;
        }
    }
    el::Loggers::setCategories(new_categories.c_str(), true);
}

void
rtclog_set_log_level(int level)
{
    const char *categories = get_default_categories(level);
    rtclog_set_categories(categories);
}

void
rtclog_set_log(const char *log)
{
    long level;
    char *ptr = NULL;

    if (!*log) {
        rtclog_set_categories(log);
        return;
    }
    level = strtol(log, &ptr, 10);
    if (ptr && *ptr) {
        if (*ptr == ',') {
            std::string new_categories = std::string(get_default_categories(level)) + ptr;
            rtclog_set_categories(new_categories.c_str());
        } else {
            rtclog_set_categories(log);
        }
    } else if (level >= 0 && level <= 4) {
        rtclog_set_log_level(level);
    } else {
        // Fallback logging if rtclog not ready? Just printf for now or ignore
        printf("Invalid numerical log level: %s\n", log);
    }
}

// ----------------------------------------------------------------------
// Implementation of New Interfaces
// ----------------------------------------------------------------------

void rtclog_init(const char *name) {
    rtclog_configure(name ? std::string(name) : "", true);
}

void rtclog_set_level(enum RtcLogLevel lvl) {
    // Map internal logic level to categories logic
    // The previous logic used 0-4 levels for categories setup
    // 0,1: INFO+; 2: DEBUG; 3,4: TRACE
    
    int internal_level = 0;
    switch(lvl) {
        case RTC_FATAL: 
        case RTC_ERROR: 
        case RTC_WARN:  
        case RTC_INFO:  internal_level = 1; break;
        case RTC_DEBUG: internal_level = 2; break;
        case RTC_TRACE: internal_level = 4; break;
    }
    rtclog_set_log_level(internal_level);
}

void rtclog_set_thread_name(const char *name) {
    if (name) {
        el::Helpers::setThreadName(name);
    }
}

static el::Level rtc_level_to_el(enum RtcLogLevel lvl) {
    switch(lvl) {
        case RTC_FATAL: return el::Level::Fatal;
        case RTC_ERROR: return el::Level::Error;
        case RTC_WARN:  return el::Level::Warning;
        case RTC_INFO:  return el::Level::Info;
        case RTC_DEBUG: return el::Level::Debug;
        case RTC_TRACE: return el::Level::Trace;
        default: return el::Level::Info;
    }
}

bool rtclog_is_enabled(enum RtcLogLevel lvl) {
    return ELPP->vRegistry()->allowed(rtc_level_to_el(lvl), RTC_DEFAULT_LOG_CATEGORY);
}

void rtclog_fmt(const char *filename, int fileline, const char* funcname, enum RtcLogLevel lvl, const char *fmt, ...) {
    // Note: Enablement check is already done by the macro if used correctly,
    // but we double check here for direct calls or safety.
    el::Level el_lvl = rtc_level_to_el(lvl);
    if (!ELPP->vRegistry()->allowed(el_lvl, RTC_DEFAULT_LOG_CATEGORY)) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    
    // Optimization: Try to use a stack buffer for small messages
    char stack_buf[1024];
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(stack_buf, sizeof(stack_buf), fmt, args_copy);
    va_end(args_copy);
    
    if (len < 0) {
        va_end(args);
        return;
    }

    if (static_cast<size_t>(len) < sizeof(stack_buf)) {
        el::base::Writer(el_lvl, el::Color::Default, filename, fileline, funcname).construct(RTC_DEFAULT_LOG_CATEGORY) << stack_buf;
    } else {
        // Fallback to heap allocation for large messages
        std::string message(len, '\0');
        vsnprintf(&message[0], len + 1, fmt, args);
        el::base::Writer(el_lvl, el::Color::Default, filename, fileline, funcname).construct(RTC_DEFAULT_LOG_CATEGORY) << message;
    }
    
    va_end(args);
}
