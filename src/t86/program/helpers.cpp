#include "helpers.h"

namespace tiny::t86 {
    Register Reg(size_t index) {
        if (index > Cpu::Config::instance().registerCnt()) {
            throw InvalidRegister(Register(index));
        }
        return Register(index);
    }

    Register Pc() {
        return Register::ProgramCounter();
    }

    Register Flags() {
        return Register::Flags();
    }

    Register Sp() {
        return Register::StackPointer();
    }

    Register Bp() {
        return Register::StackBasePointer();
    }

    FloatRegister FReg(size_t index) {
        return FloatRegister(index);
    }

    Memory::Register Mem(Register reg) {
        return Memory::Register(reg);
    }

    Memory::Immediate Mem(uint64_t index) {
        return Memory::Immediate(index);
    }

    Memory::RegisterOffset Mem(const RegisterOffset& regOffset) {
        return Memory::RegisterOffset(regOffset);
    }

    Memory::RegisterRegister Mem(const RegisterRegister& regReg) {
        return Memory::RegisterRegister(regReg);
    }

    Memory::RegisterScaled Mem(const RegisterScaled& regScaled) {
        return Memory::RegisterScaled(regScaled);
    }

    Memory::RegisterOffsetRegister Mem(const RegisterOffsetRegister& regOffsetReg) {
        return Memory::RegisterOffsetRegister(regOffsetReg);
    }

    Memory::RegisterRegisterScaled Mem(const RegisterRegisterScaled& regRegScaled) {
        return Memory::RegisterRegisterScaled(regRegScaled);
    }

    Memory::RegisterOffsetRegisterScaled Mem(const RegisterOffsetRegisterScaled& regOffsetRegScaled) {
        return Memory::RegisterOffsetRegisterScaled(regOffsetRegScaled);
    }


    void printAllRegisters(const Cpu& cpu, std::ostream& os) {
        os << "Pc: " << cpu.getRegister(Pc()) << '\n';
        os << "Sp: " << cpu.getRegister(Sp()) << '\n';
        os << "Bp: " << cpu.getRegister(Bp()) << '\n';
        os << "Flags: " << cpu.getRegister(Flags()) << '\n';
        for (std::size_t i = 0; i < cpu.registersCount(); ++i) {
            os << "Reg(" << i << "): " << cpu.getRegister(Reg(i)) << '\n';
        }
        os << std::flush;
    }
}