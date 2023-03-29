#include "ram.h"
#include "cpu.h"
#include "instruction.h"

namespace tiny::t86 {

    std::size_t RAM::readLatency(std::size_t) const {
        return 5;
    }

    std::size_t RAM::writeLatency(std::size_t) const {
        return 5;
    }

    std::size_t MOV::length() const {
        // POC how this can be used
        if (!destination_.isRegister()) {
            if (!value_.isRegister()) {
                return 4;
            }
            return 3;
        }
        return 2;
    }

    std::size_t LEA::length() const {
        return 1;
    }

    std::size_t NOP::length() const {
        return 1;
    }

    std::size_t HALT::length() const {
        return 1;
    }

    std::size_t DBG::length() const {
        return 1;
    }

    std::size_t BREAK::length() const {
        return 1;
    }

    std::size_t ADD::length() const {
        return 1;
    }

    std::size_t SUB::length() const {
        return 1;
    }

    std::size_t INC::length() const {
        return 1;
    }

    std::size_t DEC::length() const {
        return 1;
    }

    std::size_t NEG::length() const {
        return 1;
    }

    std::size_t MUL::length() const {
        return 1;
    }

    std::size_t DIV::length() const {
        return 1;
    }

    std::size_t MOD::length() const {
        return 1;
    }

    std::size_t IMUL::length() const {
        return 1;
    }

    std::size_t IDIV::length() const {
        return 1;
    }

    std::size_t AND::length() const {
        return 1;
    }

    std::size_t OR::length() const {
        return 1;
    }

    std::size_t XOR::length() const {
        return 1;
    }

    std::size_t NOT::length() const {
        return 1;
    }

    std::size_t LSH::length() const {
        return 1;
    }

    std::size_t RSH::length() const {
        return 1;
    }

    std::size_t CLF::length() const {
        return 1;
    }

    std::size_t CMP::length() const {
        return 1;
    }

    std::size_t FCMP::length() const {
        return 1;
    }

    std::size_t JMP::length() const {
        return 1;
    }

    std::size_t LOOP::length() const {
        return 1;
    }

    std::size_t JZ::length() const {
        return 1;
    }

    std::size_t JNZ::length() const {
        return 1;
    }

    std::size_t JE::length() const {
        return 1;
    }

    std::size_t JNE::length() const {
        return 1;
    }

    std::size_t JG::length() const {
        return 1;
    }

    std::size_t JGE::length() const {
        return 1;
    }

    std::size_t JL::length() const {
        return 1;
    }

    std::size_t JLE::length() const {
        return 1;
    }

    std::size_t JA::length() const {
        return 1;
    }

    std::size_t JAE::length() const {
        return 1;
    }

    std::size_t JB::length() const {
        return 1;
    }

    std::size_t JBE::length() const {
        return 1;
    }

    std::size_t JO::length() const {
        return 1;
    }

    std::size_t JNO::length() const {
        return 1;
    }

    std::size_t JS::length() const {
        return 1;
    }

    std::size_t JNS::length() const {
        return 1;
    }

    std::size_t CALL::length() const {
        return 1;
    }

    std::size_t RET::length() const {
        return 1;
    }

    std::size_t PUSH::length() const {
        return 1;
    }

    std::size_t FPUSH::length() const {
        return 1;
    }

    std::size_t POP::length() const {
        return 1;
    }

    std::size_t FPOP::length() const {
        return 1;
    }

    std::size_t PUTCHAR::length() const {
        return 1;
    }

    std::size_t PUTNUM::length() const {
        return 1;
    }

    std::size_t GETCHAR::length() const {
        return 1;
    }

    std::size_t FADD::length() const {
        return 1;
    }

    std::size_t FSUB::length() const {
        return 1;
    }

    std::size_t FMUL::length() const {
        return 1;
    }

    std::size_t FDIV::length() const {
        return 1;
    }

    std::size_t EXT::length() const {
        return 1;
    }

    std::size_t NRW::length() const {
        return 1;
    }

}