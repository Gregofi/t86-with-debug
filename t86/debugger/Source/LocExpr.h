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
    int value;
};

using Operand = std::variant<Register, Integer>;

struct Push {
    Operand value;
};

/// Pops two values off the stack, adds them and
/// pushes them back.
struct Add {
};

/// Pops two values off the stack and does
/// `first pop` - `second pop`.
struct Subst {
};

struct FrameBaseRegisterOffset {
    int offset;
};

using LocExpr = std::variant<Push, Add, Subst, FrameBaseRegisterOffset>;
}
