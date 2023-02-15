#include <fmt/core.h>
#include <fmt/color.h>


template<typename... Args>
inline void log_info(fmt::format_string<Args...> format_str, Args&& ...args) {
#ifdef LOGGER_INFO
    fmt::print(fg(fmt::color::blue), "INFO: ");
    fmt::print(format_str, args...);
#endif
}

template<typename... Args>
inline void log_error(fmt::format_string<Args...> format_str, Args&& ...args) {
    fmt::print(fg(fmt::color::crimson), "ERROR: ");
    fmt::print(format_str, args...);
}


