#pragma once

#include <string>
#include <sstream>
#include <memory>
#include <iostream>
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

}
