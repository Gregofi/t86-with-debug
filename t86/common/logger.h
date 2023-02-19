#pragma once

#include <fmt/core.h>
#include <fmt/color.h>


template<typename... Args>
inline void log_debug(fmt::format_string<Args...> format_str, Args&& ...args) {
#if LOG_LEVEL > 3
    fmt::print(stderr, fg(fmt::color::blue), "DEBUG: ");
    fmt::print(stderr, format_str, args...);
    fmt::print(stderr, "\n");
#endif
}

template<typename... Args>
inline void log_info(fmt::format_string<Args...> format_str, Args&& ...args) {
#if LOG_LEVEL > 2
    fmt::print(stderr, fg(fmt::color::blue), "INFO: ");
    fmt::print(stderr, format_str, args...);
    fmt::print(stderr, "\n");
#endif
}

template<typename... Args>
inline void log_warning(fmt::format_string<Args...> format_str, Args&& ...args) {
#if LOG_LEVEL > 1
    fmt::print(stderr, fg(fmt::color::orange), "WARNING: ");
    fmt::print(stderr, format_str, args...);
    fmt::print(stderr, "\n");
#endif
}

template<typename... Args>
inline void log_error(fmt::format_string<Args...> format_str, Args&& ...args) {
#if LOG_LEVEL > 0
    fmt::print(stderr, fg(fmt::color::crimson), "ERROR: ");
    fmt::print(stderr, format_str, args...);
    fmt::print(stderr, "\n");
#endif
}


