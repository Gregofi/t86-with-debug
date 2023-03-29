#include "operand.h"
#include "../cpu/memory.h"

#include <cassert>


namespace tiny::t86 {

    Operand::Operand(int64_t value) : value_(value) {}

    Operand::Operand(double value) : value_(value) {}

    Operand::Operand(const Register& reg) : value_(reg) {}

    Operand::Operand(const RegisterOffset& regOff) : value_(regOff) {}

    Operand::Operand(const RegisterRegister& regReg) : value_(regReg) {}

    Operand::Operand(const RegisterScaled& regScaled) : value_(regScaled) {}

    Operand::Operand(const RegisterOffsetRegister& regOffReg) : value_(regOffReg) {}

    Operand::Operand(const RegisterRegisterScaled& regRegScaled) : value_(regRegScaled) {}

    Operand::Operand(const RegisterOffsetRegisterScaled& regOffRegScaled) : value_(regOffRegScaled) {};

    Operand::Operand(const Memory::Immediate& address) : value_(address) {}

    Operand::Operand(const Memory::Register& address) : value_(address) {}

    Operand::Operand(const Memory::RegisterOffset& address) : value_(address) {}

    Operand::Operand(const Memory::RegisterRegister& address) : value_(address) {}

    Operand::Operand(const Memory::RegisterScaled& address) : value_(address) {}

    Operand::Operand(const Memory::RegisterOffsetRegister& address) : value_(address) {}

    Operand::Operand(const Memory::RegisterRegisterScaled& address) : value_(address) {}

    Operand::Operand(const Memory::RegisterOffsetRegisterScaled& address) : value_(address) {}

    Operand::Operand(const FloatRegister& fReg) : value_(fReg) {}

    int64_t Operand::getValue() const {
        assert(isValue() || isFloatValue());
        if (isFloatValue()) {
            double fVal = getFloatValue();
            return *reinterpret_cast<int64_t*>(&fVal);
        }
        return std::get<int64_t>(value_);
    }

    double Operand::getFloatValue() const {
        assert(isFloatValue() || isValue());
        if (isValue()) {
            int64_t val = getValue();
            return *reinterpret_cast<double*>(&val);
        }
        return std::get<double>(value_);
    }

    void Operand::supply(int64_t val) {
        assert(!isValue());
        if (isRegister()) {
            value_ = supply(getRegister(), val);
        } else if (isRegisterOffset()) {
            value_ = supply(getRegisterOffset(), val);
        } else if (isRegisterRegister()) {
            value_ = supply(getRegisterRegister(), val);
        } else if (isRegisterScaled()) {
            value_ = supply(getRegisterScaled(), val);
        } else if (isRegisterOffsetRegister()) {
            value_ = supply(getRegisterOffsetRegister(), val);
        } else if (isRegisterRegisterScaled()) {
            value_ = supply(getRegisterRegisterScaled(), val);
        } else if (isRegisterOffsetRegisterScaled()) {
            value_ = supply(getRegisterOffsetRegisterScaled(), val);
        } else if (isMemoryImmediate()) {
            value_ = supply(getMemoryImmediate(), val);
        } else if (isMemoryRegister()) {
            value_ = supply(getMemoryRegister(), val);
        } else if (isMemoryRegisterOffset()) {
            value_ = supply(getMemoryRegisterOffset(), val);
        } else if (isMemoryRegisterRegister()) {
            value_ = supply(getMemoryRegisterRegister(), val);
        } else if (isMemoryRegisterScaled()) {
            value_ = supply(getMemoryRegisterScaled(), val);
        } else if (isMemoryRegisterOffsetRegister()) {
            value_ = supply(getMemoryRegisterOffsetRegister(), val);
        } else if (isMemoryRegisterRegisterScaled()) {
            value_ = supply(getMemoryRegisterRegisterScaled(), val);
        } else if (isMemoryRegisterOffsetRegisterScaled()) {
            value_ = supply(getMemoryRegisterOffsetRegisterScaled(), val);
        } else {
            throw std::runtime_error("Unhandled operand type");
        }
    }

    void Operand::supply(double val) {
        if (isFloatRegister()) {
            value_ = supply(getFloatRegister(), val);
        } else {
            throw std::runtime_error("Unhandled operand type");
        }
    }

    bool Operand::isValue() const {
        return std::holds_alternative<int64_t>(value_);
    }

    bool Operand::isFloatValue() const {
        return std::holds_alternative<double>(value_);
    }

    bool Operand::isRegister() const {
        return std::holds_alternative<Register>(value_);
    }

    bool Operand::isRegisterOffset() const {
        return std::holds_alternative<RegisterOffset>(value_);
    }

    bool Operand::isRegisterRegister() const {
        return std::holds_alternative<RegisterRegister>(value_);
    }

    bool Operand::isRegisterScaled() const {
        return std::holds_alternative<RegisterScaled>(value_);
    }

    bool Operand::isRegisterOffsetRegister() const {
        return std::holds_alternative<RegisterOffsetRegister>(value_);
    }

    bool Operand::isRegisterRegisterScaled() const {
        return std::holds_alternative<RegisterRegisterScaled>(value_);
    }

    bool Operand::isRegisterOffsetRegisterScaled() const {
        return std::holds_alternative<RegisterOffsetRegisterScaled>(value_);
    }

    bool Operand::isMemoryImmediate() const {
        return std::holds_alternative<Memory::Immediate>(value_);
    }

    bool Operand::isMemoryRegister() const {
        return std::holds_alternative<Memory::Register>(value_);
    }

    bool Operand::isMemoryRegisterOffset() const {
        return std::holds_alternative<Memory::RegisterOffset>(value_);
    }

    bool Operand::isMemoryRegisterRegister() const {
        return std::holds_alternative<Memory::RegisterRegister>(value_);
    }

    bool Operand::isMemoryRegisterScaled() const {
        return std::holds_alternative<Memory::RegisterScaled>(value_);
    }

    bool Operand::isMemoryRegisterOffsetRegister() const {
        return std::holds_alternative<Memory::RegisterOffsetRegister>(value_);
    }

    bool Operand::isMemoryRegisterRegisterScaled() const {
        return std::holds_alternative<Memory::RegisterRegisterScaled>(value_);
    }

    bool Operand::isMemoryRegisterOffsetRegisterScaled() const {
        return std::holds_alternative<Memory::RegisterOffsetRegisterScaled>(value_);
    }

    bool Operand::isFloatRegister() const {
        return std::holds_alternative<FloatRegister>(value_);
    }

    const Register& Operand::getRegister() const {
        assert(isRegister());
        return std::get<Register>(value_);
    }

    const RegisterOffset& Operand::getRegisterOffset() const {
        assert(isRegisterOffset());
        return std::get<RegisterOffset>(value_);
    }

    const RegisterRegister& Operand::getRegisterRegister() const {
        assert(isRegisterRegister());
        return std::get<RegisterRegister>(value_);
    }

    const RegisterScaled& Operand::getRegisterScaled() const {
        assert(isRegisterScaled());
        return std::get<RegisterScaled>(value_);
    }

    const RegisterOffsetRegister& Operand::getRegisterOffsetRegister() const {
        assert(isRegisterOffsetRegister());
        return std::get<RegisterOffsetRegister>(value_);
    }

    const RegisterRegisterScaled& Operand::getRegisterRegisterScaled() const {
        assert(isRegisterRegisterScaled());
        return std::get<RegisterRegisterScaled>(value_);
    }

    const RegisterOffsetRegisterScaled& Operand::getRegisterOffsetRegisterScaled() const {
        assert(isRegisterOffsetRegisterScaled());
        return std::get<RegisterOffsetRegisterScaled>(value_);
    }

    const Memory::Immediate& Operand::getMemoryImmediate() const {
        assert(isMemoryImmediate());
        return std::get<Memory::Immediate>(value_);
    }

    const Memory::Register& Operand::getMemoryRegister() const {
        assert(isMemoryRegister());
        return std::get<Memory::Register>(value_);
    }

    const Memory::RegisterOffset& Operand::getMemoryRegisterOffset() const {
        assert(isMemoryRegisterOffset());
        return std::get<Memory::RegisterOffset>(value_);
    }

    const Memory::RegisterRegister& Operand::getMemoryRegisterRegister() const {
        assert(isMemoryRegisterRegister());
        return std::get<Memory::RegisterRegister>(value_);
    }

    const Memory::RegisterScaled& Operand::getMemoryRegisterScaled() const {
        assert(isMemoryRegisterScaled());
        return std::get<Memory::RegisterScaled>(value_);
    }

    const Memory::RegisterOffsetRegister& Operand::getMemoryRegisterOffsetRegister() const {
        assert(isMemoryRegisterOffsetRegister());
        return std::get<Memory::RegisterOffsetRegister>(value_);
    }

    const Memory::RegisterRegisterScaled& Operand::getMemoryRegisterRegisterScaled() const {
        assert(isMemoryRegisterRegisterScaled());
        return std::get<Memory::RegisterRegisterScaled>(value_);
    }

    const Memory::RegisterOffsetRegisterScaled& Operand::getMemoryRegisterOffsetRegisterScaled() const {
        assert(isMemoryRegisterOffsetRegisterScaled());
        return std::get<Memory::RegisterOffsetRegisterScaled>(value_);
    }

    const FloatRegister& Operand::getFloatRegister() const {
        assert(isFloatRegister());
        return std::get<FloatRegister>(value_);
    }

    bool Operand::isFetched() const {
        return isValue() || isFloatValue();
    }

    Requirement Operand::requirement() const {
        assert(!isValue());
        if (isRegister()) {
            return RegisterRead(getRegister());
        } else if (isRegisterOffset()) {
            return RegisterRead(getRegisterOffset().reg());
        } else if (isRegisterRegister()) {
            return RegisterRead(getRegisterRegister().reg1());
        } else if (isRegisterScaled()) {
            return RegisterRead(getRegisterScaled().reg());
        } else if (isRegisterOffsetRegister()) {
            return RegisterRead(getRegisterOffsetRegister().regOffset().reg());
        } else if (isRegisterRegisterScaled()) {
            return RegisterRead(getRegisterRegisterScaled().regScaled().reg());
        } else if (isRegisterOffsetRegisterScaled()) {
            return RegisterRead(getRegisterOffsetRegisterScaled().regScaled().reg());
        } else if (isMemoryImmediate()) {
            return MemoryRead(getMemoryImmediate().index());
        } else if (isMemoryRegister()) {
            return RegisterRead(getMemoryRegister().reg());
        } else if (isMemoryRegisterOffset()) {
            return RegisterRead(getMemoryRegisterOffset().regOffset().reg());
        } else if (isMemoryRegisterRegister()) {
            return RegisterRead(getMemoryRegisterRegister().regReg().reg1());
        } else if (isMemoryRegisterScaled()) {
            return RegisterRead(getMemoryRegisterScaled().regScaled().reg());
        } else if (isMemoryRegisterOffsetRegister()) {
            return RegisterRead(getMemoryRegisterOffsetRegister().regOffsetReg().regOffset().reg());
        } else if (isMemoryRegisterRegisterScaled()) {
            return RegisterRead(getMemoryRegisterRegisterScaled().regRegScaled().regScaled().reg());
        } else if (isMemoryRegisterOffsetRegisterScaled()) {
            return RegisterRead(getMemoryRegisterOffsetRegisterScaled().regOffsetRegScaled().regScaled().reg());
        } else if (isFloatRegister()) {
            return FloatRegisterRead(getFloatRegister());
        }
        throw std::runtime_error("Missing operand type");
    }

    Operand::Type Operand::getType() const {
        if (isValue()) {
            return Type::Imm;
        } else if (isRegister()) {
            return Type::Reg;
        } else if (isRegisterOffset()) {
            return Type::RegImm;
        } else if (isRegisterRegister()) {
            return Type::RegReg;
        } else if (isRegisterScaled()) {
            return Type::RegScaled;
        } else if (isRegisterOffsetRegister()) {
            return Type::RegImmReg;
        } else if (isRegisterRegisterScaled()) {
            return Type::RegRegScaled;
        } else if (isRegisterOffsetRegisterScaled()) {
            return Type::RegImmRegScaled;
        } else if (isMemoryImmediate()) {
            return Type::MemImm;
        } else if (isMemoryRegister()) {
            return Type::MemReg;
        } else if (isMemoryRegisterOffset()) {
            return Type::MemRegImm;
        } else if (isMemoryRegisterRegister()) {
            return Type::MemRegReg;
        } else if (isMemoryRegisterScaled()) {
            return Type::MemRegScaled;
        } else if (isMemoryRegisterOffsetRegister()) {
            return Type::MemRegImmReg;
        } else if (isMemoryRegisterRegisterScaled()) {
            return Type::MemRegRegScaled;
        } else if (isMemoryRegisterOffsetRegisterScaled()) {
            return Type::MemRegImmRegScaled;
        } else if (isFloatValue()) {
            return Type::FImm;
        } else if (isFloatRegister()) {
            return Type::FReg;
        }
        throw std::runtime_error("Unhandled operand type");
    }

    std::string Operand::typeToString(Operand::Type type) {
        switch (type) {
            case Type::Imm:
                return "Imm";
            case Type::Reg:
                return "Reg";
            case Type::RegImm:
                return "Reg + Imm";
            case Type::RegReg:
                return "Reg + Reg";
            case Type::RegScaled:
                return "Reg * Imm";
            case Type::RegImmReg:
                return "Reg + Imm + Reg";
            case Type::RegRegScaled:
                return "Reg + Reg * Imm";
            case Type::RegImmRegScaled:
                return "Reg + Imm + Reg * Imm";
            case Type::MemImm:
                return "[Imm]";
            case Type::MemReg:
                return "[Reg]";
            case Type::MemRegImm:
                return "[Reg + Imm]";
            case Type::MemRegReg:
                return "[Reg + Reg]";
            case Type::MemRegScaled:
                return "[Reg * Imm]";
            case Type::MemRegImmReg:
                return "[Reg + Imm + Reg]";
            case Type::MemRegRegScaled:
                return "[Reg + Reg * Imm]";
            case Type::MemRegImmRegScaled:
                return "[Reg + Imm + Reg * Imm]";
            case Type::FImm:
                return "FImm";
            case Type::FReg:
                return "FReg";
        }
        throw std::runtime_error("Unhandled operand type");
    }

    std::string Operand::toString() const {
        if (isValue()) {
            return std::to_string(getValue());
        } else if (isRegister()) {
            return getRegister().toString();
        } else if (isRegisterOffset()) {
            return getRegisterOffset().toString();
        } else if (isRegisterRegister()) {
            return getRegisterRegister().toString();
        } else if (isRegisterScaled()) {
            return getRegisterScaled().toString();
        } else if (isRegisterOffsetRegister()) {
            return getRegisterOffsetRegister().toString();
        } else if (isRegisterRegisterScaled()) {
            return getRegisterRegisterScaled().toString();
        } else if (isRegisterOffsetRegisterScaled()) {
            return getRegisterOffsetRegisterScaled().toString();
        } else if (isMemoryImmediate()) {
            return getMemoryImmediate().toString();
        } else if (isMemoryRegister()) {
            return getMemoryRegister().toString();
        } else if (isMemoryRegisterOffset()) {
            return getMemoryRegisterOffset().toString();
        } else if (isMemoryRegisterRegister()) {
            return getMemoryRegisterRegister().toString();
        } else if (isMemoryRegisterScaled()) {
            return getMemoryRegisterScaled().toString();
        } else if (isMemoryRegisterOffsetRegister()) {
            return getMemoryRegisterOffsetRegister().toString();
        } else if (isMemoryRegisterRegisterScaled()) {
            return getMemoryRegisterRegisterScaled().toString();
        } else if (isMemoryRegisterOffsetRegisterScaled()) {
            return getMemoryRegisterOffsetRegisterScaled().toString();
        } else if (isFloatValue()) {
            return std::to_string(getFloatValue());
        } else if (isFloatRegister()) {
            return getFloatRegister().toString();
        }
        throw std::runtime_error("Unhandled operand type");
    }

    int64_t Operand::supply(const Register&, int64_t val) {
        return val;
    }

    int64_t Operand::supply(const RegisterOffset& reg, int64_t val) {
        return val + reg.offset();
    }

    RegisterOffset Operand::supply(const RegisterRegister& reg, int64_t val) {
        return RegisterOffset(reg.reg2(), val);
    }

    int64_t Operand::supply(const RegisterScaled& reg, int64_t val) {
        return val * reg.scale();
    }

    RegisterOffset Operand::supply(const RegisterOffsetRegister& reg, int64_t val) {
        return RegisterOffset(reg.reg(), reg.regOffset().offset() + val);
    }

    RegisterOffset Operand::supply(const RegisterRegisterScaled& reg, int64_t val) {
        return RegisterOffset(reg.reg(), reg.regScaled().scale() * val);
    }

    RegisterOffset Operand::supply(const RegisterOffsetRegisterScaled& reg, int64_t val) {
        return RegisterOffset(reg.regOffset().reg(), reg.regOffset().offset() + reg.regScaled().scale() * val);
    }

    int64_t Operand::supply(const Memory::Immediate&, int64_t val) {
        return val;
    }

    Memory::Immediate Operand::supply(const Memory::Register& mem, int64_t val) {
        return Memory::Immediate(val);
    }

    Memory::Immediate Operand::supply(const Memory::RegisterOffset& mem, int64_t val) {
        return Memory::Immediate(supply(mem.regOffset(), val));
    }

    Memory::RegisterOffset Operand::supply(const Memory::RegisterRegister& mem, int64_t val) {
        return Memory::RegisterOffset(supply(mem.regReg(), val));
    }

    Memory::Immediate Operand::supply(const Memory::RegisterScaled& mem, int64_t val) {
        return Memory::Immediate(supply(mem.regScaled(), val));
    }

    Memory::RegisterOffset Operand::supply(const Memory::RegisterOffsetRegister& mem, int64_t val) {
        return Memory::RegisterOffset(supply(mem.regOffsetReg(), val));
    }

    Memory::RegisterOffset Operand::supply(const Memory::RegisterRegisterScaled& mem, int64_t val) {
        return Memory::RegisterOffset(supply(mem.regRegScaled(), val));
    }

    Memory::RegisterOffset Operand::supply(const Memory::RegisterOffsetRegisterScaled& mem, int64_t val) {
        return Memory::RegisterOffset(supply(mem.regOffsetRegScaled(), val));
    }
    double Operand::supply(const FloatRegister&, double val) {
        return val;
    }
}
