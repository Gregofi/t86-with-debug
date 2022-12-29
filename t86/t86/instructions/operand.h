#pragma once

#include "requirements.h"
#include "../cpu/register.h"
#include "../cpu/memory.h"

#include <variant>
#include <cstdint>

namespace tiny::t86 {
    /**
     * General instruction operand definition
     * Some instructions don't allow all of there operands by having specific constructors
     */
    class Operand {
    public:
        enum class Type {
            Imm, // 1
            Reg, // R0
            RegImm, // R0 + 1
            RegReg, // R0 + R1
            RegScaled, // R0 * 2
            RegImmReg, // R0 + 1 + R1
            RegRegScaled, // R0 + R1 * 2
            RegImmRegScaled, // R0 + 1 + R1 * 2
            MemImm, // [10]
            MemReg, // [R0]
            MemRegImm, // [R0 + 1]
            MemRegReg, // [R0 + R1]
            MemRegScaled, // [R0 * 2]
            MemRegImmReg, // [R0 + 1 + R1]
            MemRegRegScaled, // [R0 + R1 * 2]
            MemRegImmRegScaled, // [R0 1 + R1 * 2]
            FImm, // 4.2
            FReg, // FR0
        };

        static std::string typeToString(Type type);

        std::string toString() const;

        Operand(int64_t value);

        explicit Operand(double value);

        Operand(const Register& reg);

        Operand(const RegisterOffset& regOff);

        Operand(const RegisterRegister& regReg);

        Operand(const RegisterScaled& regScaled);

        Operand(const RegisterOffsetRegister& regOffReg);

        Operand(const RegisterRegisterScaled& regRegScaled);

        Operand(const RegisterOffsetRegisterScaled& regOffRegScaled);

        Operand(const Memory::Immediate& address);

        Operand(const Memory::Register& address);

        Operand(const Memory::RegisterOffset& address);

        Operand(const Memory::RegisterRegister& address);

        Operand(const Memory::RegisterScaled& address);

        Operand(const Memory::RegisterOffsetRegister& address);

        Operand(const Memory::RegisterRegisterScaled& address);

        Operand(const Memory::RegisterOffsetRegisterScaled& address);

        Operand(const FloatRegister& fReg);

        bool isFetched() const;

        Requirement requirement() const;

        void supply(int64_t value);

        void supply(double value);

        bool isValue() const;

        bool isFloatValue() const;

        bool isRegister() const;

        bool isRegisterOffset() const;

        bool isRegisterRegister() const;

        bool isRegisterScaled() const;

        bool isRegisterOffsetRegister() const;

        bool isRegisterRegisterScaled() const;

        bool isRegisterOffsetRegisterScaled() const;

        bool isMemoryImmediate() const;

        bool isMemoryRegister() const;

        bool isMemoryRegisterOffset() const;

        bool isMemoryRegisterRegister() const;

        bool isMemoryRegisterScaled() const;

        bool isMemoryRegisterOffsetRegister() const;

        bool isMemoryRegisterRegisterScaled() const;

        bool isMemoryRegisterOffsetRegisterScaled() const;

        bool isFloatRegister() const;

        Type getType() const;

        int64_t getValue() const;

        double getFloatValue() const;

        const Register& getRegister() const;

        const RegisterOffset& getRegisterOffset() const;

        const RegisterRegister& getRegisterRegister() const;

        const RegisterScaled& getRegisterScaled() const;

        const RegisterOffsetRegister& getRegisterOffsetRegister() const;

        const RegisterRegisterScaled& getRegisterRegisterScaled() const;

        const RegisterOffsetRegisterScaled& getRegisterOffsetRegisterScaled() const;

        const Memory::Immediate& getMemoryImmediate() const;

        const Memory::Register& getMemoryRegister() const;

        const Memory::RegisterOffset& getMemoryRegisterOffset() const;

        const Memory::RegisterRegister& getMemoryRegisterRegister() const;

        const Memory::RegisterScaled& getMemoryRegisterScaled() const;

        const Memory::RegisterOffsetRegister& getMemoryRegisterOffsetRegister() const;

        const Memory::RegisterRegisterScaled& getMemoryRegisterRegisterScaled() const;

        const Memory::RegisterOffsetRegisterScaled& getMemoryRegisterOffsetRegisterScaled() const;

        const FloatRegister& getFloatRegister() const;

        auto getOperandVar() const { return value_; };

        static int64_t supply(const Register& reg, int64_t val);

        static int64_t supply(const RegisterOffset& reg, int64_t val);

        static RegisterOffset supply(const RegisterRegister& reg, int64_t val);

        static int64_t supply(const RegisterScaled& reg, int64_t val);

        static RegisterOffset supply(const RegisterOffsetRegister& reg, int64_t val);

        static RegisterOffset supply(const RegisterRegisterScaled& reg, int64_t val);

        static RegisterOffset supply(const RegisterOffsetRegisterScaled& reg, int64_t val);

        static int64_t supply(const Memory::Immediate& mem, int64_t val);

        static Memory::Immediate supply(const Memory::Register& mem, int64_t val);

        static Memory::Immediate supply(const Memory::RegisterOffset& mem, int64_t val);

        static Memory::RegisterOffset supply(const Memory::RegisterRegister& mem, int64_t val);

        static Memory::Immediate supply(const Memory::RegisterScaled& mem, int64_t val);

        static Memory::RegisterOffset supply(const Memory::RegisterOffsetRegister& mem, int64_t val);

        static Memory::RegisterOffset supply(const Memory::RegisterRegisterScaled& mem, int64_t val);

        static Memory::RegisterOffset supply(const Memory::RegisterOffsetRegisterScaled& mem, int64_t val);

        static double supply(const FloatRegister&, double val);
    private:

        std::variant<int64_t, double, Register, FloatRegister,
                     RegisterOffset, RegisterRegister, RegisterScaled,
                     RegisterOffsetRegister, RegisterRegisterScaled, RegisterOffsetRegisterScaled,
                     Memory::Immediate, Memory::Register, Memory::RegisterOffset, Memory::RegisterRegister, Memory::RegisterScaled,
                     Memory::RegisterOffsetRegister, Memory::RegisterRegisterScaled, Memory::RegisterOffsetRegisterScaled> value_;
    };
}
