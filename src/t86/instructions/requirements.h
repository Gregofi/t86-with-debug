#pragma once

#include "../cpu/register.h"

#include <variant>
#include <cstdint>

namespace tiny::t86 {
    class RegisterRead {
    public:
        RegisterRead(Register reg) : reg_(reg) {}

        Register reg() const {
            return reg_;
        }

    private:
        Register reg_;
    };

    class FloatRegisterRead {
    public:
        FloatRegisterRead(FloatRegister fReg) : fReg_(fReg) {}

        FloatRegister fReg() const {
            return fReg_;
        }

    private:
        FloatRegister fReg_;
    };


    class MemoryRead {
    public:
        MemoryRead(uint64_t addr) : addr_(addr) {}

        uint64_t addr() const {
            return addr_;
        }

    private:
        uint64_t addr_;
    };

    /**
     * Representation of requirement of operand
     * This is used to determine what cpu supplies
     */
    class Requirement {
    public:
        Requirement(RegisterRead regRead) : value_(regRead) {}

        Requirement(FloatRegisterRead fRegRead) : value_(fRegRead) {}
            
        Requirement(MemoryRead memRead) : value_(memRead) {}

        bool isRegisterRead() const;

        bool isFloatRegisterRead() const;

        bool isMemoryRead() const;

        Register getRegisterRead() const;

        FloatRegister getFloatRegisterRead() const;

        uint64_t getMemoryRead() const;

    private:
        std::variant<RegisterRead, FloatRegisterRead, MemoryRead> value_;
    };
}
