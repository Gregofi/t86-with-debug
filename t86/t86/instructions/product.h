#pragma once

#include <variant>

#include "../cpu/memory.h"
#include "../cpu/register.h"
#include "operand.h"

namespace tiny::t86 {
class Product {
public:
    Product(Register reg)
        : product_(reg) { }

    Product(FloatRegister fReg)
        : product_(fReg) { }

    Product(Memory::Immediate mem)
        : product_(mem) { }

    Product(Memory::Register)
        : product_(MemoryRegister()) { }

    static Product fromOperand(Operand op);

    bool isRegister() const;

    Register getRegister() const;

    bool isFloatRegister() const;

    FloatRegister getFloatRegister() const;

    bool isMemoryImmediate() const;

    Memory::Immediate getMemoryImmediate() const;

    bool isMemoryRegister() const;

private:
    // Helper struct representing "unknown memory address based on register(s)"
    struct MemoryRegister { };

    Product(MemoryRegister memReg)
        : product_(memReg) { }

    std::variant<Register, FloatRegister, Memory::Immediate, MemoryRegister>
        product_;
};
} // namespace tiny::t86
