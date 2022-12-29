#pragma once

#include <map>

#include "register.h"

namespace tiny::t86 {
    class Cpu;

    class RegisterAllocationTable {
    public:
        // The number of logical registers here is passed so we don't have to worry
        // if cpu's register count is already initialized
        RegisterAllocationTable(Cpu& cpu, std::size_t registerCnt, std::size_t floatRegisterCnt);

        RegisterAllocationTable(const RegisterAllocationTable& other);

        RegisterAllocationTable(RegisterAllocationTable&& other);

        ~RegisterAllocationTable();

        RegisterAllocationTable& operator=(const RegisterAllocationTable& other);

        void rename(Register from, PhysicalRegister to);

        void rename(FloatRegister from, PhysicalRegister to);

        PhysicalRegister translate(Register reg) const;

        PhysicalRegister translate(FloatRegister fReg) const;

        bool isUnmapped(PhysicalRegister reg) const;

    protected:
        void subscribeToReads();

        void unsubscribeFromReads();

        std::map<std::variant<Register, FloatRegister>, PhysicalRegister> table_;

        Cpu& cpu_;
    };
}
