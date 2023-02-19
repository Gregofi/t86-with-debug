#pragma once

#include <string>
#include <sstream>
#include <memory>
#include <iostream>
#include <vector>
#include <string_view>

#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()

#define MARK_AS_UNUSED(ARG_NAME) ((void)(ARG_NAME))

#define UNREACHABLE throw std::runtime_error(STR("Unreachable code at " << __FILE__ << ": " << __LINE__))
#define NOT_IMPLEMENTED throw std::runtime_error(STR("Not implemented code at " << __FILE__ << ":" << __LINE__))

namespace tiny {
    template<typename T>
    std::unique_ptr<T> to_unique(T* ptr) { return std::unique_ptr<T>{ptr}; }
}

namespace utils {
    /// Splits given string_view by the delimiter.
    /// Lifetime of the splitted strings is bound to
    /// lifetime of the string which was viewed by 's'.
    /// To use this outside of that lifetime, convert
    /// the string_views to strings.
    inline std::vector<std::string_view> split(std::string_view s, char delim) {
        std::vector<std::string_view> result;
        while (!s.empty()) {
            auto size = s.find(delim);
            result.emplace_back(s.substr(size));
            s.remove_prefix(size);
        }
        return result;
    }
}
