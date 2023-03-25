#pragma once

#include <numeric>
#include <cctype>
#include <string>
#include <sstream>
#include <memory>
#include <iostream>
#include <variant>
#include <vector>
#include <string_view>
#include <charconv>
#include <optional>
#include <functional>

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
    inline std::vector<std::string_view> split_v(std::string_view s, char delim = ' ') {
        std::vector<std::string_view> result;
        while (!s.empty()) {
            auto size = s.find(delim);
            if (size == s.npos) {
                result.emplace_back(s);
                break;
            }
            if (size != 0) {
                result.emplace_back(s.substr(0, size));
            }
            s.remove_prefix(size + 1);
        }
        return result;
    }

    /// Same as split_v but returns vector of strings instead of views.
    inline std::vector<std::string> split(std::string_view s, char delim = ' ') {
        auto splitted = split_v(s, delim);
        std::vector<std::string> res;
        std::transform(splitted.begin(), splitted.end(), std::back_inserter(res), [](auto &&v) { return std::string(v); });
        return res;
    }

    template<typename InputIt>
    inline std::string join(InputIt begin, InputIt end, const std::string& j = " ") {
        std::string res;
        for (; begin < end; ++begin) {
            res += *begin;
            if (std::next(begin, 1) != end) {
                res += j;
            }
        }
        return res;
    }

    inline std::optional<int64_t> svtoi64(std::string_view s) {
        int64_t result;
        auto [ptr, ec] { std::from_chars(s.data(), s.data() + s.size(), result) };
        if (ec == std::errc()) {
            return result;
        }
        return std::nullopt;
    }

    /// from_chars wrapper that converts 's' to T.
    template<typename T>
    inline std::optional<T> svtonum(std::string_view s) {
#ifdef __clang__
        if constexpr (std::is_floating_point_v<T>) {
            try {
                auto res = std::stod(std::string{s});
                return res;
            } catch (...) {
                return std::nullopt;
            }
        } else {
#endif
            T result;
            auto [ptr, ec] { std::from_chars(s.data(), s.data() + s.size(), result) };
            if (ec == std::errc()) {
                return result;
            }
            return std::nullopt;
#ifdef __clang__
        }
#endif
    }

    inline bool is_prefix_of(std::string_view prefix, std::string_view of) {
        if (prefix.size() > of.size()) {
            return false;
        }
        for (size_t i = 0; i < prefix.size(); i++) {
            if (prefix[i] != of[i]) {
                return false;
            }
        }
        return true;
    }

    inline std::string squash_strip_whitespace(std::string_view s) {
        std::string result;
        bool last_space = true;
        while (!s.empty() && std::isspace(s.back())) {
            s.remove_suffix(1);
        }
        while (!s.empty() && std::isspace(s.front())) {
            s.remove_prefix(1);
        }
        for (const auto c: s) {
            if (!std::isspace(c)) {
                result.push_back(c);
                last_space = false;
            } else if (!last_space) {
                result.push_back(c);
                last_space = true;
            }
        }
        return result;
    }
    /// Variant utils
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
}
