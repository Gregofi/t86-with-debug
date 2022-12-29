#pragma once

#include "register.h"

#include <cstdlib>
#include <variant>
#include <optional>


namespace tiny::t86::Memory {
    class Immediate {
    public:
        explicit Immediate(uint64_t index) : index_(index) {}

        size_t index() const { return index_; }

        std::string toString() const {
            return "[" + std::to_string(index_) + "]";
        }

    private:
        size_t index_;
    };

    class Register {
    public:
        explicit Register(t86::Register reg) : register_(reg) {}

        t86::Register reg() const { return register_; }

        std::string toString() const {
            return "[" + register_.toString() + "]";
        }
    private:
        t86::Register register_;
    };

    class RegisterOffset {
    public:
        explicit RegisterOffset(t86::RegisterOffset regOffset) : regOffset_(regOffset) {}

        std::string toString() const {
            return "[" + regOffset_.toString() + "]";
        }

        t86::RegisterOffset regOffset() const {
            return regOffset_;
        }

    protected:
        t86::RegisterOffset regOffset_;
    };

    class RegisterScaled {
    public:
        explicit RegisterScaled(t86::RegisterScaled regScaled) : regScaled_(regScaled) {}

        std::string toString() const {
            return "[" + regScaled_.toString() + "]";
        }

        t86::RegisterScaled regScaled() const {
            return regScaled_;
        }

    protected:
        t86::RegisterScaled regScaled_;
    };

    class RegisterRegister {
    public:
        explicit RegisterRegister(t86::RegisterRegister regReg) : regReg_(regReg) {}

        std::string toString() const {
            return "[" + regReg_.toString() + "]";
        }

        t86::RegisterRegister regReg() const {
            return regReg_;
        }

    protected:
        t86::RegisterRegister regReg_;
    };

    class RegisterOffsetRegister {
    public:
        explicit RegisterOffsetRegister(t86::RegisterOffsetRegister regOffReg) : regOffsetReg_(regOffReg) {}

        std::string toString() const {
            return "[" + regOffsetReg_.toString() + "]";
        }

        t86::RegisterOffsetRegister regOffsetReg() const {
            return regOffsetReg_;
        }

    protected:
        t86::RegisterOffsetRegister regOffsetReg_;
    };

    class RegisterRegisterScaled {
    public:
        explicit RegisterRegisterScaled(t86::RegisterRegisterScaled regRegScaled) : regRegScaled_(regRegScaled) {}

        std::string toString() const {
            return "[" + regRegScaled_.toString() + "]";
        }

        t86::RegisterRegisterScaled regRegScaled() const {
            return regRegScaled_;
        }

    protected:
        t86::RegisterRegisterScaled regRegScaled_;
    };

    class RegisterOffsetRegisterScaled {
    public:
        explicit RegisterOffsetRegisterScaled(t86::RegisterOffsetRegisterScaled regOffRegScaled) : regOffsetRegScaled_(regOffRegScaled) {}

        std::string toString() const {
            return "[" + regOffsetRegScaled_.toString() + "]";
        }

        t86::RegisterOffsetRegisterScaled regOffsetRegScaled() const {
            return regOffsetRegScaled_;
        }

    protected:
        t86::RegisterOffsetRegisterScaled regOffsetRegScaled_;
    };
}
