#pragma once
#include <utility>
#include <variant>
#include <climits>
#include <string>
#include <optional>
#include <compare>
#include "../../fmt/format.h"

class Immediate {
public:
    Immediate(int val): val(val) { }
    std::string toString() const { return fmt::format("{}", val); }
    int val;
};

class Register {
public:
    enum class Type : unsigned long {
        SP = ULONG_MAX - 1,
        BP = ULONG_MAX - 2,
        IP = ULONG_MAX - 3,
    };
    explicit Register(std::size_t idx): idx(idx) { }
    explicit Register(Type type): idx(static_cast<std::size_t>(type)) { }
    auto operator<=>(const Register& other) const = default;
    std::string toString() const { 
        std::string res = fmt::format("R{}", idx);
        if (idx == (size_t)Type::SP) {
            res = "SP";
        } else if (idx == (size_t)Type::BP) {
            res = "BP";
        } else if (idx == (size_t)Type::IP) {
            res = "IP";
        }
        return res; 
    }
    std::size_t idx;
};

class RegisterOffset {
public:
    RegisterOffset(Register reg, int offset): reg(reg), offset(offset) { }
    auto operator<=>(const RegisterOffset& other) const = default;
    std::string toString() const { return fmt::format("{} + {}", reg.toString(), offset); }
    Register reg;
    int offset;
};

class Memory {
public:
    explicit Memory(std::size_t idx): idx(idx) { }
    auto operator<=>(const Memory& other) const = default;
    std::string toString() const { return fmt::format("[{}]", idx); }
    std::size_t idx;
};

class MemoryRegister {
public:
    explicit MemoryRegister(Register reg): reg(reg) { }
    auto operator<=>(const MemoryRegister& other) const = default;
    std::string toString() const { return fmt::format("[{}]", reg.toString()); }
    Register reg;
};

class MemoryRegisterOffset {
public:
    explicit MemoryRegisterOffset(Register reg, int offset): reg(reg), offset(offset) { }
    auto operator<=>(const MemoryRegisterOffset& other) const = default;
    std::string toString() const { return fmt::format("[{} + {}]", reg.toString(), offset); }
    Register reg;
    int offset;
};

class Label {
public:
    Label(): dest() { }
    explicit Label(std::string dest): dest(std::move(dest)) { }
    std::string toString() const { return "LBL"; }
    std::optional<std::string> dest;
};

using Operand = std::variant<Immediate, Register, Memory, MemoryRegister, MemoryRegisterOffset, Label, RegisterOffset>;