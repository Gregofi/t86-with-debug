#pragma once

#include <string>
#include <sstream>
#include <memory>
#include <iostream>

#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()

#define MARK_AS_UNUSED(ARG_NAME) ((void)(ARG_NAME))

#define UNREACHABLE throw std::runtime_error(STR("Unreachable code at " << __FILE__ << ": " << __LINE__))
#define NOT_IMPLEMENTED throw std::runtime_error(STR("Not implemented code at " << __FILE__ << ":" << __LINE__))

namespace tiny {
    template<typename T>
    std::unique_ptr<T> to_unique(T * ptr) { return std::unique_ptr<T>{ptr}; }
}

namespace utils {
    template<typename...>
    static void output(std::ostream& os, std::string_view format_str) {
        os << format_str;
    }

    template<typename H, typename... T>
    static void output(std::ostream& os, std::string_view format_str, const H& arg, T... args) {
        auto it = format_str.find("{}");
        if (it == format_str.npos) {
            os << format_str;
            return;
        }
        os << format_str.substr(0, it);
        os << arg;
        output(os, format_str.substr(it + 2), args...);
    }


    template<typename...>
    static void logger(std::string_view format_str) {
#ifdef LOGGER
        std::cerr << "LOGGER: " << format_str << std::endl;
#endif
    }

    template<typename H, typename... T>
    static void logger(std::string_view format_str, const H& arg, T... args) {
#ifdef LOGGER
        std::cerr << "LOGGER: ";
        output(std::cerr, format_str, arg, args...);
        std::cerr << std::endl;
#endif
    }

    template<typename... Ts>
    std::string format(std::string_view format_str, Ts... args) {
        std::ostringstream oss;
        output(oss, format_str, args...);
        return oss.str();
    }
}
