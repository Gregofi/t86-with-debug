#include <cassert>
#include <utility>
#include <sstream>

#include "program/label.h"
#include "instruction.h"
#include "cpu.h"
#include "cpu/reservation_station.h"
#include "cpu/register.h"

namespace tiny::t86 {
    std::string Instruction::typeToString(Instruction::Type type) {
        switch (type) {
            case Type::MOV:
                return "MOV";
            case Type::LEA:
                return "LEA";
            case Type::NOP:
                return "NOP";
            case Type::HALT:
                return "HALT";
            case Type::DBG:
                return "DBG";
            case Type::BREAK:
                return "BREAK";
            case Type::ADD:
                return "ADD";
            case Type::SUB:
                return "SUB";
            case Type::INC:
                return "INC";
            case Type::DEC:
                return "DEC";
            case Type::NEG:
                return "NEG";
            case Type::MUL:
                return "MUL";
            case Type::DIV:
                return "DIV";
            case Type::MOD:
                return "MOD";
            case Type::IMUL:
                return "IMUL";
            case Type::IDIV:
                return "IDIV";
            case Type::AND:
                return "AND";
            case Type::OR:
                return "OR";
            case Type::XOR:
                return "XOR";
            case Type::NOT:
                return "NOT";
            case Type::LSH:
                return "LSH";
            case Type::RSH:
                return "RSH";
//            case Type::LRL:
//                return "LRL";
//            case Type::RRL:
//                return "RRL";
            case Type::CLF:
                return "CLF";
            case Type::CMP:
                return "CMP";
            case Type::FCMP:
                return "FCMP";
            case Type::JMP:
                return "JMP";
            case Type::LOOP:
                return "LOOP";
            case Type::JZ:
                return "JZ";
            case Type::JNZ:
                return "JNZ";
            case Type::JE:
                return "JE";
            case Type::JNE:
                return "JNE";
            case Type::JG:
                return "JG";
            case Type::JGE:
                return "JGE";
            case Type::JL:
                return "JL";
            case Type::JLE:
                return "JLE";
            case Type::JA:
                return "JA";
            case Type::JAE:
                return "JAE";
            case Type::JB:
                return "JB";
            case Type::JBE:
                return "JBE";
            case Type::JO:
                return "JO";
            case Type::JNO:
                return "JNO";
            case Type::JS:
                return "JS";
            case Type::JNS:
                return "JNS";
            case Type::CALL:
                return "CALL";
            case Type::RET:
                return "RET";
            case Type::PUSH:
                return "PUSH";
            case Type::FPUSH:
                return "FPUSH";
            case Type::POP:
                return "POP";
            case Type::FPOP:
                return "FPOP";
            case Type::PUTCHAR:
                return "PUTCHAR";
            case Type::PUTNUM:
                return "PUTNUM";
            case Type::GETCHAR:
                return "GETCHAR";
            case Type::FADD:
                return "FADD";
            case Type::FSUB:
                return "FSUB";
            case Type::FMUL:
                return "FMUL";
            case Type::FDIV:
                return "FDIV";
            case Type::EXT:
                return "EXT";
            case Type::NRW:
                return "NRW";
        }
        throw std::runtime_error("Unhandled instruction type");
    }

    Instruction::Signature Instruction::getSignature() const {
        auto operands = signatureOperands();
        std::vector<Operand::Type> operandTypes;
        operandTypes.reserve(operands.size());
        for (const auto& operand : operands) {
            operandTypes.push_back(operand.getType());
        }
        return { type(), operandTypes };
    }

    std::string Instruction::toString() const {
        std::ostringstream ss;
        ss << typeToString(type());
        auto operands = signatureOperands();
        auto it = operands.begin();
        if (it != operands.end()) {
            ss << " " + it->toString();
            for (++it; it != operands.end(); ++it) {
                ss << ", " + it->toString();
            }
        }
        return ss.str();
    }

    std::string Instruction::Signature::toString() const {
        std::ostringstream ss;
        ss << typeToString(type);
        auto it = operandTypes.begin();
        if (it != operandTypes.end()) {
            ss << " " + Operand::typeToString(*it);
            for (++it; it != operandTypes.end(); ++it) {
                ss << ", " + Operand::typeToString(*it);
            }
        }
        return ss.str();
    }

    bool Instruction::Signature::operator<(const Instruction::Signature& other) const {
        return std::tie(type, operandTypes) < std::tie(other.type, other.operandTypes);
    }

    bool Instruction::Signature::operator==(const Signature& other) const {
        return type == other.type && operandTypes == other.operandTypes;
    }

    void BinaryArithmeticInstruction::validate() const {
        if (reg_.isSpecial()) {
            throw InvalidOperand(reg_);
        }
    }

    void BinaryArithmeticInstruction::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 2);
        Alu::Result binOpRes = op_(operands[0].getValue(), operands[1].getValue());
        entry.setRegister(dest_, binOpRes.value);
        entry.setFlags(binOpRes.flags);
    }

#define BINARY_ARITH_INS_IMPL(INS_NAME, OP)                                                                     \
    INS_NAME::INS_NAME(Register reg, Register val) : BinaryArithmeticInstruction(OP, reg, val) {}               \
    INS_NAME::INS_NAME(Register reg, RegisterOffset regDisp) : BinaryArithmeticInstruction(OP, reg, regDisp) {} \
    INS_NAME::INS_NAME(Register reg, int64_t val) : BinaryArithmeticInstruction(OP, reg, val) {}                \
    INS_NAME::INS_NAME(Register reg, Memory::Immediate val) : BinaryArithmeticInstruction(OP, reg, val) {}      \
    INS_NAME::INS_NAME(Register reg, Memory::Register val) : BinaryArithmeticInstruction(OP, reg, val) {}       \
    INS_NAME::INS_NAME(Register reg, Memory::RegisterOffset val) : BinaryArithmeticInstruction(OP, reg, val) {} \
    INS_NAME::INS_NAME(Register reg, Operand val) : BinaryArithmeticInstruction(OP, reg, val) {} \
    INS_NAME::INS_NAME(Register dest, Register reg, int64_t val) : BinaryArithmeticInstruction(OP, dest, reg, val) {} \
    INS_NAME::INS_NAME(Register dest, Register reg, Register val) : BinaryArithmeticInstruction(OP, dest, reg, val) {}

    BINARY_ARITH_INS_IMPL(MOD, &Alu::mod)

    BINARY_ARITH_INS_IMPL(ADD, &Alu::add)

    BINARY_ARITH_INS_IMPL(SUB, &Alu::subtract)

    BINARY_ARITH_INS_IMPL(MUL, &Alu::multiply)

    BINARY_ARITH_INS_IMPL(DIV, &Alu::divide)

    BINARY_ARITH_INS_IMPL(IMUL, &Alu::signed_multiply)

    BINARY_ARITH_INS_IMPL(IDIV, &Alu::signed_divide)

    BINARY_ARITH_INS_IMPL(AND, &Alu::bit_and)

    BINARY_ARITH_INS_IMPL(OR, &Alu::bit_or)

    BINARY_ARITH_INS_IMPL(XOR, &Alu::bit_xor)

    BINARY_ARITH_INS_IMPL(LSH, &Alu::bit_left_shift)

    BINARY_ARITH_INS_IMPL(RSH, &Alu::bit_right_shift)

    //BINARY_ARITH_INS_IMPL(LRL, &Alu::bit_left_roll)

    //BINARY_ARITH_INS_IMPL(RRL, &Alu::bit_right_roll)



    void FloatBinaryArithmeticInstruction::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 2);
        Alu::FloatResult binOpRes = op_(operands[0].getFloatValue(), operands[1].getFloatValue());
        entry.setFloatRegister(fReg_, binOpRes.value);
        entry.setFlags(binOpRes.flags);
    }

#define FLOAT_BINARY_ARITH_INS_IMPL(INS_NAME, OP)                                                        \
    INS_NAME::INS_NAME(FloatRegister fReg, double val) : FloatBinaryArithmeticInstruction(OP, fReg, val) {}        \
    INS_NAME::INS_NAME(FloatRegister fReg, FloatRegister val) : FloatBinaryArithmeticInstruction(OP, fReg, val) {}

    FLOAT_BINARY_ARITH_INS_IMPL(FADD, &Alu::fadd)
    FLOAT_BINARY_ARITH_INS_IMPL(FSUB, &Alu::fsubtract)
    FLOAT_BINARY_ARITH_INS_IMPL(FMUL, &Alu::fmultiply)
    FLOAT_BINARY_ARITH_INS_IMPL(FDIV, &Alu::fdivide)

    void UnaryArithmeticInstruction::validate() const {
        if (reg_.isSpecial()) {
            throw InvalidOperand(reg_);
        }
    }

    void UnaryArithmeticInstruction::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 1);
        Alu::Result res = op_(operands[0].getValue());
        entry.setRegister(reg_, res.value);
        entry.setFlags(res.flags);
    }

#define UNARY_ARITH_INS_IMPL(INS_NAME, OP)                                                           \
    INS_NAME::INS_NAME(Register reg) : UnaryArithmeticInstruction([](int64_t value){ return OP; }, reg) {}

    UNARY_ARITH_INS_IMPL(INC, Alu::add(value, 1))

    UNARY_ARITH_INS_IMPL(DEC, Alu::subtract(value, 1))

    UNARY_ARITH_INS_IMPL(NEG, Alu::negate(value))

    UNARY_ARITH_INS_IMPL(NOT, Alu::bit_not(value))

    std::vector<Operand> MOV::operands() const {
        if (destination_.isRegister() || destination_.isMemoryImmediate() || destination_.isFloatRegister()) {
            return {value_};
        }
        else if (destination_.isMemoryRegister()) {
            return {value_, destination_.getMemoryRegister().reg()};
        }
        else if (destination_.isMemoryRegisterOffset()) {
            return {value_, destination_.getMemoryRegisterOffset().regOffset().reg()};
        }
        else if (destination_.isMemoryRegisterScaled()) {
            return {value_, destination_.getMemoryRegisterScaled().regScaled().reg()};
        }
        else if (destination_.isMemoryRegisterRegister()) {
            const auto& regReg = destination_.getMemoryRegisterRegister().regReg();
            return {value_, regReg.reg1(), regReg.reg2()};
        }
        else if (destination_.isMemoryRegisterOffsetRegister()) {
            const auto& regOffsetReg = destination_.getMemoryRegisterOffsetRegister().regOffsetReg();
            return {value_, regOffsetReg.regOffset().reg(), regOffsetReg.reg()};
        }
        else if (destination_.isMemoryRegisterRegisterScaled()) {
            const auto& regRegScaled = destination_.getMemoryRegisterRegisterScaled().regRegScaled();
            return {value_, regRegScaled.regScaled().reg(), regRegScaled.reg()};
        }
        else if (destination_.isMemoryRegisterOffsetRegisterScaled()) {
            const auto& regOffsetRegScaled = destination_.getMemoryRegisterOffsetRegisterScaled().regOffsetRegScaled();
            return {value_, regOffsetRegScaled.regScaled().reg(), regOffsetRegScaled.regOffset().reg()};
        }
        throw std::runtime_error("Unhandled destination type");
    }

    void MOV::retire(ReservationStation::Entry& entry) const {
        // Register write is already taken care of in execute function
        // no need to take care of it here
        if (destination_.isRegister() || destination_.isFloatRegister()) {
            return;
        } else {
        // Make sure memory write happens
        const auto& memoryWriteIds = entry.memoryWriteIds();
        assert(memoryWriteIds.size() == 1);
        entry.writeMemory(memoryWriteIds[0]);
        }
    }

    void MOV::validate() const {
        if (destination_.isRegister()) {
            const auto& reg = destination_.getRegister();
            if (reg.isSpecial()) {
                throw InvalidOperand(reg);
            }
        }
    }

    void MOV::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(!operands.empty());
        if (destination_.isRegister()) {
            assert(operands.size() == 1);
            entry.setRegister(destination_.getRegister(), operands[0].getValue());
        } else if (destination_.isFloatRegister()) {
            assert(operands.size() == 1);
            entry.setFloatRegister(destination_.getFloatRegister(), operands[0].getFloatValue());
        } else if (destination_.isMemoryImmediate()) {
            assert(operands.size() == 1);
            const auto& memoryWriteIds = entry.memoryWriteIds();
            assert(memoryWriteIds.size() == 1);
            entry.setWriteValue(memoryWriteIds[0], operands[0].getValue());
        } else {
            const auto& memoryWriteIds = entry.memoryWriteIds();
            assert(memoryWriteIds.size() == 1);
            int64_t address;
            if (destination_.isMemoryRegister()) {
                assert(operands.size() == 2);
                address = Operand::supply(destination_.getMemoryRegister(), operands[1].getValue()).index();
            }
            else if (destination_.isMemoryRegisterOffset()) {
                assert(operands.size() == 2);
                address = Operand::supply(destination_.getMemoryRegisterOffset(), operands[1].getValue()).index();
            }
            else if (destination_.isMemoryRegisterScaled()) {
            assert(operands.size() == 2);
            address = Operand::supply(destination_.getMemoryRegisterScaled(), operands[1].getValue()).index();
            }
            else if (destination_.isMemoryRegisterRegister()) {
                assert(operands.size() == 3);
                address = Operand::supply(
                        Operand::supply(destination_.getMemoryRegisterRegister(), operands[1].getValue()),
                        operands[2].getValue()
                        ).index();
            }
            else if (destination_.isMemoryRegisterOffsetRegister()) {
                assert(operands.size() == 3);
                address = Operand::supply(
                        Operand::supply(destination_.getMemoryRegisterOffsetRegister(), operands[1].getValue()),
                        operands[2].getValue()
                        ).index();
            }
            else if (destination_.isMemoryRegisterRegisterScaled()) {
                assert(operands.size() == 3);
                address = Operand::supply(
                        Operand::supply(destination_.getMemoryRegisterRegisterScaled(), operands[1].getValue()),
                        operands[2].getValue()
                        ).index();
            }
            else if (destination_.isMemoryRegisterOffsetRegisterScaled()) {
                assert(operands.size() == 3);
                address = Operand::supply(
                        Operand::supply(destination_.getMemoryRegisterOffsetRegisterScaled(), operands[1].getValue()),
                        operands[2].getValue()
                        ).index();
            }
            else {
                throw std::runtime_error("Unhandled operand type");
            }
            entry.specifyWriteAddress(memoryWriteIds[0], address);
            entry.setWriteValue(memoryWriteIds[0], operands[0].getValue());
        }
    }

    void CLF::retire(ReservationStation::Entry& entry) {
        entry.setFlags(Alu::Flags{false, false, false, false});
    }

    void ConditionalJumpInstruction::retire(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 2);
        entry.processJump(condition_(operands[1].getValue()));
    }

    void ConditionalJumpInstruction::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 2);
        if (condition_(operands[1].getValue())) {
            entry.setProgramCounter(operands[0].getValue());
        }
    }

    void JMP::retire(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 1);
        entry.processJump(true);
    }

    void JMP::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 1);
        entry.setProgramCounter(operands[0].getValue());
    }

#define COND_JMP_INS_IMPL(INS_NAME, CONDITION) \
INS_NAME::INS_NAME(Register address) : ConditionalJumpInstruction([](Alu::Flags flags) { return CONDITION; }, address) {} \
INS_NAME::INS_NAME(uint64_t address) : ConditionalJumpInstruction([](Alu::Flags flags){ return CONDITION; }, address) {} \
INS_NAME::INS_NAME(Memory::Immediate address) : ConditionalJumpInstruction([](Alu::Flags flags) { return CONDITION; }, address) {} \
INS_NAME::INS_NAME(Memory::Register address) : ConditionalJumpInstruction([](Alu::Flags flags) { return CONDITION; }, address) {} \
INS_NAME::INS_NAME(Memory::RegisterOffset address) : ConditionalJumpInstruction([](Alu::Flags flags){ return CONDITION; }, address) {} \
INS_NAME::INS_NAME(Operand address) : ConditionalJumpInstruction([](Alu::Flags flags){ return CONDITION; }, address) {}

    COND_JMP_INS_IMPL(JZ, flags.zeroFlag)

    COND_JMP_INS_IMPL(JE, flags.zeroFlag)

    COND_JMP_INS_IMPL(JNZ, !flags.zeroFlag)

    COND_JMP_INS_IMPL(JNE, !flags.zeroFlag)

    COND_JMP_INS_IMPL(JG, !flags.zeroFlag && (flags.signFlag == flags.overflowFlag))

    COND_JMP_INS_IMPL(JGE, flags.signFlag == flags.overflowFlag)

    COND_JMP_INS_IMPL(JL, flags.signFlag != flags.overflowFlag)

    COND_JMP_INS_IMPL(JLE, flags.zeroFlag || flags.signFlag != flags.overflowFlag)

    COND_JMP_INS_IMPL(JA, !(flags.carryFlag || flags.zeroFlag))

    COND_JMP_INS_IMPL(JAE, !flags.carryFlag)

    COND_JMP_INS_IMPL(JB, flags.carryFlag)

    COND_JMP_INS_IMPL(JBE, flags.carryFlag || flags.zeroFlag)

    COND_JMP_INS_IMPL(JO, flags.overflowFlag)

    COND_JMP_INS_IMPL(JNO, !flags.overflowFlag)

    COND_JMP_INS_IMPL(JS, flags.signFlag)

    COND_JMP_INS_IMPL(JNS, !flags.signFlag)

    void CMP::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 2);
        Alu::Result res = Alu::subtract(operands[0].getValue(), operands[1].getValue());
        entry.setFlags(res.flags);
    }

    void FCMP::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 2);
        Alu::FloatResult res = Alu::fsubtract(operands[0].getFloatValue(), operands[1].getFloatValue());
        entry.setFlags(res.flags);
    }

    void LOOP::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 2);
        Alu::Result res = Alu::subtract(operands[0].getValue(), 1);
        entry.setRegister(reg_, res.value);
        entry.setFlags(res.flags);
        entry.operands().emplace_back(res.value);
        if (res.value != 0) {
            entry.setProgramCounter(operands[1].getValue());
        }
    }

    void LOOP::retire(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 3);
        entry.processJump(operands[2].getValue() != 0);
    }

    void PUSH::retire(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        const auto& memWriteIds = entry.memoryWriteIds();
        assert(operands.size() == 2);
        assert(memWriteIds.size() == 1);
        entry.writeMemory(memWriteIds[0]);
    }

    void PUSH::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        const auto& memWriteIds = entry.memoryWriteIds();
        assert(operands.size() == 2);
        assert(memWriteIds.size() == 1);
        entry.specifyWriteAddress(memWriteIds[0], operands[1].getValue() - 1);
        entry.setWriteValue(memWriteIds[0], operands[0].getValue());
        entry.setStackPointer(operands[1].getValue() - 1);
    }


    void FPUSH::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        const auto& memWriteIds = entry.memoryWriteIds();
        assert(operands.size() == 2);
        assert(memWriteIds.size() == 1);
        entry.specifyWriteAddress(memWriteIds[0], operands[1].getValue() - 1);
        double opVal = operands[0].getFloatValue();
        entry.setWriteValue(memWriteIds[0], *reinterpret_cast<int64_t*>(&opVal));
        entry.setStackPointer(operands[1].getValue() - 1);
    }

    void FPUSH::retire(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        const auto& memWriteIds = entry.memoryWriteIds();
        assert(operands.size() == 2);
        assert(memWriteIds.size() == 1);
        entry.writeMemory(memWriteIds[0]);
    }

    void POP::validate() const {
        if (reg_ == Register::ProgramCounter()) {
            throw InvalidOperand(reg_);
        }
    }

    void POP::retire(ReservationStation::Entry&) const {
        // No need to anything here, if we don't overwrite old memory
    }

    void POP::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 2);
        entry.setRegister(reg_, operands[0].getValue());
        entry.setStackPointer(operands[1].getValue() + 1);
    }

    void FPOP::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 2);
        int64_t opValue = operands[0].getValue();
        entry.setFloatRegister(fReg_, *reinterpret_cast<double*>(&opValue));
        entry.setStackPointer(operands[1].getValue() + 1);
    }

    void FPOP::retire(ReservationStation::Entry& entry) const {
        // Nothing to be done here
    }

    void CALL::retire(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        const auto& memWriteIds = entry.memoryWriteIds();
        assert(operands.size() == 3);
        assert(memWriteIds.size() == 1);
        entry.writeMemory(memWriteIds[0]);
        entry.processJump(true);
    }

    void CALL::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        const auto& memWriteIds = entry.memoryWriteIds();
        assert(operands.size() == 3);
        assert(memWriteIds.size() == 1);
        entry.setProgramCounter(operands[0].getValue());
        entry.specifyWriteAddress(memWriteIds[0], operands[2].getValue() - 1);
        entry.setWriteValue(memWriteIds[0], operands[1].getValue());
        entry.setStackPointer(operands[2].getValue() - 1);
    }

    void RET::retire(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 2);
        entry.processJump(true);
    }

    void RET::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 2);
        entry.setProgramCounter(operands[0].getValue());
        entry.setStackPointer(operands[1].getValue() + 1);
    }

    void DBG::retire(ReservationStation::Entry& entry) const {
        entry.unrollSpeculation();
        debugFunction_(entry.cpu());
    }

    void BREAK::retire(ReservationStation::Entry& entry) const {
        entry.unrollSpeculation();
        entry.cpu().doBreak();
    }

    void HALT::retire(ReservationStation::Entry& entry) const {
        entry.unrollSpeculation();
        entry.cpu().halt();
    }

    void PatchableJumpInstruction::setDestination(uint64_t address) {
        assert(address_.isValue() && static_cast<uint64_t>(address_.getValue()) == Label::empty()
               && "You can change jump destination only on empty labeled jumps");
        address_ = address;
    }

    void LEA::validate() const {
        if (reg_.isSpecial() || reg_ == Register::StackPointer() || reg_ == Register::StackBasePointer()) {
            throw InvalidOperand(reg_);
        }
    }

    std::vector<Operand> LEA::operands() const {
        if (mem_.isMemoryRegisterOffset()) {
            return {mem_.getMemoryRegisterOffset().regOffset().reg()};
        }
        else if (mem_.isMemoryRegisterScaled()) {
            return {mem_.getMemoryRegisterScaled().regScaled().reg()};
        }
        else if (mem_.isMemoryRegisterRegister()) {
            const auto& regReg = mem_.getMemoryRegisterRegister().regReg();
            return {regReg.reg1(), regReg.reg2()};
        }
        else if (mem_.isMemoryRegisterOffsetRegister()) {
            const auto& regOffsetReg = mem_.getMemoryRegisterOffsetRegister().regOffsetReg();
            return {regOffsetReg.regOffset().reg(), regOffsetReg.reg()};
        }
        else if (mem_.isMemoryRegisterRegisterScaled()) {
            const auto& regRegScaled = mem_.getMemoryRegisterRegisterScaled().regRegScaled();
            return {regRegScaled.regScaled().reg(), regRegScaled.reg()};
        }
        else if (mem_.isMemoryRegisterOffsetRegisterScaled()) {
            const auto& regOffsetRegScaled = mem_.getMemoryRegisterOffsetRegisterScaled().regOffsetRegScaled();
            return {regOffsetRegScaled.regScaled().reg(), regOffsetRegScaled.regOffset().reg()};
        }
        throw std::runtime_error("Unhandled operand type");
    }

    void LEA::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        int64_t address;
        if (mem_.isMemoryRegisterOffset()) {
            assert(operands.size() == 1);
            address = Operand::supply(mem_.getMemoryRegisterOffset(), operands[0].getValue()).index();
        }
        else if (mem_.isMemoryRegisterScaled()) {
            assert(operands.size() == 1);
            address = Operand::supply(mem_.getMemoryRegisterScaled(), operands[0].getValue()).index();
        }
        else if (mem_.isMemoryRegisterRegister()) {
            assert(operands.size() == 2);
            address = Operand::supply(
                    Operand::supply(mem_.getMemoryRegisterRegister(), operands[0].getValue()),
                    operands[2].getValue()
            ).index();
        }
        else if (mem_.isMemoryRegisterOffsetRegister()) {
            assert(operands.size() == 2);
            address = Operand::supply(
                    Operand::supply(mem_.getMemoryRegisterOffsetRegister(), operands[0].getValue()),
                    operands[1].getValue()
            ).index();
        }
        else if (mem_.isMemoryRegisterRegisterScaled()) {
            assert(operands.size() == 2);
            address = Operand::supply(
                    Operand::supply(mem_.getMemoryRegisterRegisterScaled(), operands[0].getValue()),
                    operands[1].getValue()
            ).index();
        }
        else if (mem_.isMemoryRegisterOffsetRegisterScaled()) {
            assert(operands.size() == 2);
            address = Operand::supply(
                    Operand::supply(mem_.getMemoryRegisterOffsetRegisterScaled(), operands[0].getValue()),
                    operands[1].getValue()
            ).index();
        }
        else {
            throw std::runtime_error("Unhandled operand type");
        }
        entry.setRegister(reg_, address);
    }

    void PUTCHAR::retire(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 1);
        os_ << static_cast<char>(operands[0].getValue()) << std::flush;
    }

    void PUTNUM::retire(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 1);
        os_ << static_cast<int>(operands[0].getValue()) << std::endl;
    }

    void GETCHAR::retire(ReservationStation::Entry& entry) const {
        char c;
        is_ >> c;
        entry.setRegister(reg_, c);
    }

    void EXT::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 1);
        entry.setFloatRegister(fReg_, static_cast<double>(operands[0].getValue()));
    }

    void NRW::execute(ReservationStation::Entry& entry) const {
        const auto& operands = entry.operands();
        assert(operands.size() == 1);
        entry.setRegister(reg_, static_cast<int64_t>(operands[0].getFloatValue()));
    }
}
