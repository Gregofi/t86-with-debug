#pragma once

#include <cstddef>
#include <iostream>

#include "../cpu.h"
#include "../cpu/register.h"
#include "../cpu/memory.h"
#include "../cpu/memory.h"

namespace tiny::t86 {
    Register Reg(size_t index);

    Register Pc();

    Register Flags();

    Register Sp();

    Register Bp();

    FloatRegister FReg(size_t index);

    // [reg]
    Memory::Register Mem(Register reg);

    // [imm]
    Memory::Immediate Mem(uint64_t index);

    // [reg + displacement]
    Memory::RegisterOffset Mem(const RegisterOffset& regOffset);

    // [reg + reg]
    Memory::RegisterRegister Mem(const RegisterRegister& regReg);

    // [reg * imm]
    Memory::RegisterScaled Mem(const RegisterScaled& regScaled);

    // [reg + imm + reg]
    Memory::RegisterOffsetRegister Mem(const RegisterOffsetRegister& regOffsetReg);

    // [reg + reg * imm]
    Memory::RegisterRegisterScaled Mem(const RegisterRegisterScaled& regRegScaled);

    // [reg + imm + reg * imm]
    Memory::RegisterOffsetRegisterScaled Mem(const RegisterOffsetRegisterScaled& regOffsetRegScaled);

    void printAllRegisters(const Cpu& cpu, std::ostream& os = std::cout);
}
