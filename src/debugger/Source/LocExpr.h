#pragma once
#include "fmt/core.h"
#include "helpers.h"
#include <string>
#include <variant>

/// Describes the location in the executing code.
/// Is a stack virtual machine, the last value
/// remaining on the stack is the location.
namespace expr {

struct Register {
    std::string name;
};

struct Offset {
    int64_t value;
};

using Location = std::variant<Register, Offset>;

inline std::string LocationToStr(const Location& loc) {
    return std::visit(utils::overloaded {
        [](const Register& reg) {
            return reg.name;
        },
        [](const Offset& offset) {
            return fmt::format("[{}]", offset.value);
        }
    }, loc);
}

struct Push {
    Location value;
};

/// Pops two values off the stack, adds them and
/// pushes them back.
struct Add {
};

struct FrameBaseRegisterOffset {
    int64_t offset;
};

struct Dereference {
    uint64_t location;
};

using LocExpr = std::variant<Push, Add, FrameBaseRegisterOffset, Dereference>;
}
