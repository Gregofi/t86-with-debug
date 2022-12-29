#include "requirements.h"

#include <cassert>

namespace tiny::t86 {

    bool Requirement::isRegisterRead() const {
        return std::holds_alternative<RegisterRead>(value_);
    }

    bool Requirement::isFloatRegisterRead() const {
        return std::holds_alternative<FloatRegisterRead>(value_);
    }

    Register Requirement::getRegisterRead() const {
        assert(isRegisterRead());
        return std::get<RegisterRead>(value_).reg();
    }

    FloatRegister Requirement::getFloatRegisterRead() const {
        assert(isFloatRegisterRead());
        return std::get<FloatRegisterRead>(value_).fReg();
    }

    bool Requirement::isMemoryRead() const {
        return std::holds_alternative<MemoryRead>(value_);
    }

    uint64_t Requirement::getMemoryRead() const {
        assert(isMemoryRead());
        return std::get<MemoryRead>(value_).addr();
    }
}
