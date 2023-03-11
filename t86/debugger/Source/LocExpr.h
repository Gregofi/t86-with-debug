#pragma once
#include <string>
#include <variant>
/// Describes the location in the executing code.
/// Is a stack virtual machine, the last value
/// remaining on the stack is the location.

namespace expr {

struct Register {
    std::string name;
};

struct Integer {
    int64_t value;
};

using Operand = std::variant<Register, Integer>;

struct Push {
    Operand value;
};

/// Pops two values off the stack, adds them and
/// pushes them back.
struct Add {
};

struct FrameBaseRegisterOffset {
    int64_t offset;
};

using LocExpr = std::variant<Push, Add, FrameBaseRegisterOffset>;
}
