#include <string>
#include <utility>
#include <fmt/core.h>

class Logger {
public:
    Logger() = delete;
    Logger(const Logger&) = delete;

    template<typename... Args>
    static void log(const fmt::format_string<Args...> &format_string, Args&&... args) {
#ifdef __DEBUG__
        fmt::print(stderr, format_string, std::forward<Args>(args)...);
#endif
    }
};
