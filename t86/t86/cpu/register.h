#pragma once

#include <cstddef>
#include <limits>
#include <functional>
#include <string>
#include <stdexcept>
#include <variant>

namespace tiny::t86 {
    class RegisterOffset;
    class RegisterScaled;
    class RegisterRegister;
    class RegisterOffsetRegister;
    class RegisterRegisterScaled;
    class RegisterOffsetRegisterScaled;

    // This one will not be a "full" operand, only for ease of writing RegisterOffsetRegisterScaled
    class RegisterScaledOffset;

    /** The register does not store its value, but it is a register descriptor that tells me the index of the register only.
     * This represents the logical register (those, that is used in ISA)
     */
    class Register {
    public:
        constexpr static Register ProgramCounter() {
            return Register{std::numeric_limits<size_t>::max()};
        }

        constexpr static Register StackPointer() {
            return Register{std::numeric_limits<size_t>::max() - 1};
        }

        constexpr static Register StackBasePointer() {
            return Register{std::numeric_limits<size_t>::max() - 2};
        }

        constexpr static Register Flags() {
            return Register{std::numeric_limits<size_t>::max() - 3};
        }

        constexpr Register(size_t index) : index_{index} {}

        constexpr size_t index() const {
            return index_;
        }

        bool isSpecial() const {
            switch (index_) {
                case ProgramCounter().index_:
                case Flags().index_:
                    return true;
                default:
                    return false;
            }
        }

        RegisterOffset operator+(int64_t offset) const;

        RegisterOffset operator-(int64_t offset) const;

        RegisterRegister operator+(Register reg) const;

        RegisterRegisterScaled operator-(Register reg) const;

        RegisterScaled operator*(int64_t scale) const;

        RegisterRegisterScaled operator+(RegisterScaled regScaled) const;

        RegisterRegisterScaled operator-(RegisterScaled regScaled) const;

        RegisterOffsetRegister operator+(RegisterOffset regOffset) const;

        bool operator<(Register other) const {
            return index_ < other.index_;
        }

        bool operator==(Register other) const {
            return index_ == other.index_;
        }

        bool operator!=(Register other) const {
            return index_ != other.index_;
        }

        std::string toString() const {
            switch (index_) {
                case ProgramCounter().index_:
                    return "Pc";
                case StackPointer().index_:
                    return "Sp";
                case StackBasePointer().index_:
                    return "Bp";
                case Flags().index_:
                    return "Flags";
                default:
                    return "Reg" + std::to_string(index_);
            }
        }

    private:
        size_t index_;
    };

    // Operand type
    class RegisterOffset {
    public:
        RegisterOffset(Register reg, int64_t offset) : reg_(reg), offset_(offset) {}

        Register reg() const {
            return reg_;
        }

        int64_t offset() const {
            return offset_;
        }

        std::string toString() const {
            return reg_.toString() + " + " + std::to_string(offset_);
        }

        RegisterOffsetRegister operator+(Register reg) const;

        RegisterOffsetRegisterScaled operator-(Register reg) const;

        RegisterOffsetRegisterScaled operator+(RegisterScaled regScaled) const;

        RegisterOffsetRegisterScaled operator-(RegisterScaled regScaled) const;

    protected:
        Register reg_;
        int64_t offset_;
    };

    class RegisterScaled {
    public:
        RegisterScaled(const Register& reg, int64_t scale) : reg_(reg), scale_(scale) {};

        Register reg() const {
            return reg_;
        }

        int64_t scale() const {
            return scale_;
        }

        std::string toString() const {
            return reg_.toString() + " * " + std::to_string(scale_);
        }

        RegisterRegisterScaled operator+(Register reg) const;

        RegisterOffsetRegisterScaled operator+(RegisterOffset regOffset) const;

        RegisterScaledOffset operator+(int64_t offset) const;

        RegisterScaledOffset operator-(int64_t offset) const;
    protected:
        Register reg_;
        int64_t scale_;
    };

    class RegisterRegister {
    public:
        RegisterRegister(Register reg1, Register reg2) : reg1_(reg1), reg2_(reg2) {};

        Register reg1() const {
            return reg1_;
        }

        Register reg2() const {
            return reg2_;
        }

        std::string toString() const {
            return reg1_.toString() + " + " + reg2_.toString();
        }

        RegisterRegisterScaled operator*(int64_t scale) const;

        RegisterOffsetRegister operator+(int64_t offset) const;

        RegisterOffsetRegister operator-(int64_t offset) const;

    protected:
        Register reg1_;
        Register reg2_;
    };

    class RegisterOffsetRegister {
    public:
        RegisterOffsetRegister(RegisterOffset regOffset, Register reg) : regOffset_(regOffset), reg_(reg) {}

        Register reg() const {
            return reg_;
        }

        RegisterOffset regOffset() const {
            return regOffset_;
        }

        std::string toString() const {
            return regOffset_.toString() + " + " + reg_.toString();
        }

        RegisterOffsetRegisterScaled operator*(int64_t scale) const;

    protected:
        RegisterOffset regOffset_;
        Register reg_;
    };

    class RegisterRegisterScaled {
    public:
        RegisterRegisterScaled(Register reg, RegisterScaled regScaled) : reg_(reg), regScaled_(regScaled) {}

        Register reg() const {
            return reg_;
        }

        RegisterScaled regScaled() const {
            return regScaled_;
        }

        std::string toString() const {
            return reg_.toString() + " + " + regScaled_.toString();
        }

        RegisterOffsetRegisterScaled operator+(int64_t offset) const;

        RegisterOffsetRegisterScaled operator-(int64_t offset) const;

    protected:
        Register reg_;
        RegisterScaled regScaled_;
    };

    class RegisterOffsetRegisterScaled {
    public:
        RegisterOffsetRegisterScaled(RegisterOffset regOffset, RegisterScaled regScaled) : regOffset_(regOffset), regScaled_(regScaled) {}

        RegisterOffset regOffset() const {
            return regOffset_;
        }

        RegisterScaled regScaled() const {
            return regScaled_;
        }

        std::string toString() const {
            return regOffset_.toString() + " + " + regScaled_.toString();
        }
    protected:
        RegisterOffset regOffset_;
        RegisterScaled regScaled_;
    };

    // This one is temporary class, if you try to use this as instruction operand, this will fail
    class RegisterScaledOffset {
    public:
        RegisterScaledOffset(RegisterScaled regScaled, int64_t offset) : regScaled_(regScaled), offset_(offset) {}

        // Because this is temporary, no getters will be exposed, no one should be accessing any of them anyway

        RegisterOffsetRegisterScaled operator+(Register reg) const;
    protected:
        RegisterScaled regScaled_;
        int64_t offset_;
    };

    /**
     * Represents invalid register
     * For example this would arise if CPU has only 5 logical registers and instructions uses register 8
     */
    class InvalidRegister : public std::runtime_error {
    public:
        InvalidRegister(Register reg) : std::runtime_error("Invalid register " + reg.toString()) {}
    };


    class FloatRegister {
    public:
        FloatRegister(size_t index) : index_(index) {}

        size_t index() const {
            return index_;
        }

        bool operator<(const FloatRegister other) const {
            return index_ < other.index_;
        }

        bool operator==(const FloatRegister other) const {
            return index_ == other.index_;
        }

        std::string toString() const {
            return "FReg" + std::to_string(index_);
        }

    private:
        size_t index_;
    };

    /**
     * This does not store any value, just refers to a physical register in CPU.
     * The refered register can be either Register of FloatRegister
     */
    class PhysicalRegister {
    public:
        PhysicalRegister(size_t index) : index_{index} {}

        size_t index() const {
            return index_;
        }

        bool operator<(const PhysicalRegister& other) const {
            return index_ < other.index_;
        }

        bool operator==(const PhysicalRegister& other) const {
            return index_ == other.index_;
        }

    private:
        size_t index_;
    };
}

template<>
struct std::hash<tiny::t86::Register> {
    std::size_t operator()(const tiny::t86::Register& lr) const noexcept {
        return std::hash<size_t>()(lr.index());
    }
};

template<>
struct std::hash<tiny::t86::FloatRegister> {
    std::size_t operator()(const tiny::t86::FloatRegister& lr) const noexcept {
        return std::hash<size_t>()(lr.index());
    }
};
