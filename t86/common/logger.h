#pragma once

#include <fmt/core.h>
#include <fmt/color.h>
#include <string_view>


template<typename... Args>
inline void log_debug(std::string_view format_str, Args&& ...args) {
#if LOG_LEVEL > 3
    fmt::print(stderr, fg(fmt::color::violet), "DEBUG: ");
    fmt::print(stderr, fmt::runtime(format_str), std::forward<Args>(args)...);
    fmt::print(stderr, "\n");
#endif
}

template<typename... Args>
inline void log_info(std::string_view format_str, Args&& ...args) {
#if LOG_LEVEL > 2
    fmt::print(stderr, fg(fmt::color::blue), "INFO: ");
    fmt::print(stderr, fmt::runtime(format_str), std::forward<Args>(args)...);
    fmt::print(stderr, "\n");
#endif
}

template<typename... Args>
inline void log_warning(std::string_view format_str, Args&& ...args) {
#if LOG_LEVEL > 1
    fmt::print(stderr, fg(fmt::color::orange), "WARNING: ");
    fmt::print(stderr, fmt::runtime(format_str), std::forward<Args>(args)...);
    fmt::print(stderr, "\n");
#endif
}

template<typename... Args>
inline void log_error(std::string_view format_str, Args&& ...args) {
#if LOG_LEVEL > 0
    fmt::print(stderr, fg(fmt::color::crimson), "ERROR: ");
    fmt::print(stderr, fmt::runtime(format_str), std::forward<Args>(args)...);
    fmt::print(stderr, "\n");
#endif
}
