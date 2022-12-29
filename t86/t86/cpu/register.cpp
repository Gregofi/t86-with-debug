#include "register.h"
#include "../instructions/requirements.h"

namespace tiny::t86 {
    RegisterOffset Register::operator+(int64_t offset) const {
        return RegisterOffset(*this, offset);
    }

    RegisterOffset Register::operator-(int64_t offset) const {
        return RegisterOffset(*this, -offset);
    }

    RegisterRegister Register::operator+(Register reg) const {
        return RegisterRegister(*this, reg);
    }

    RegisterRegisterScaled Register::operator-(Register reg) const {
        return RegisterRegisterScaled(*this, reg * -1);
    }

    RegisterScaled Register::operator*(int64_t scale) const {
        return RegisterScaled(*this, scale);
    }

    RegisterRegisterScaled Register::operator+(RegisterScaled regScaled) const {
        return RegisterRegisterScaled(*this, regScaled);
    }

    RegisterRegisterScaled Register::operator-(RegisterScaled regScaled) const {
        return RegisterRegisterScaled(*this, regScaled.reg() * -regScaled.scale());
    }

    RegisterOffsetRegister Register::operator+(RegisterOffset regOffset) const {
        return RegisterOffsetRegister(regOffset, *this);
    }

    RegisterOffsetRegister RegisterOffset::operator+(Register reg) const {
        return RegisterOffsetRegister(*this , reg);
    }

    RegisterOffsetRegisterScaled RegisterOffset::operator-(Register reg) const {
        return RegisterOffsetRegisterScaled(*this , RegisterScaled(reg, -1));
    }

    RegisterOffsetRegisterScaled RegisterOffset::operator+(RegisterScaled regScaled) const {
        return RegisterOffsetRegisterScaled(*this, regScaled);
    }

    RegisterOffsetRegisterScaled RegisterOffset::operator-(RegisterScaled regScaled) const {
        return RegisterOffsetRegisterScaled(*this, regScaled.reg() * -regScaled.scale());
    }

    RegisterRegisterScaled RegisterScaled::operator+(Register reg) const {
        return RegisterRegisterScaled(reg, *this);
    }

    RegisterOffsetRegisterScaled RegisterScaled::operator+(RegisterOffset regOffset) const {
        return RegisterOffsetRegisterScaled(regOffset, *this);
    }

    RegisterScaledOffset RegisterScaled::operator+(int64_t offset) const {
        return RegisterScaledOffset(*this, offset);
    }

    RegisterScaledOffset RegisterScaled::operator-(int64_t offset) const {
        return RegisterScaledOffset(*this, -offset);
    }

    RegisterRegisterScaled RegisterRegister::operator*(int64_t scale) const {
        return RegisterRegisterScaled(reg1_, reg2_ * scale);
    }

    RegisterOffsetRegister RegisterRegister::operator+(int64_t offset) const {
        return RegisterOffsetRegister(reg1_ + offset, reg2_);
    }

    RegisterOffsetRegister RegisterRegister::operator-(int64_t offset) const {
        return RegisterOffsetRegister(reg1_ - offset, reg2_);
    }

    RegisterOffsetRegisterScaled RegisterOffsetRegister::operator*(int64_t scale) const {
        return RegisterOffsetRegisterScaled(regOffset_, reg_ * scale);
    }

    RegisterOffsetRegisterScaled RegisterRegisterScaled::operator+(int64_t offset) const {
        return RegisterOffsetRegisterScaled(reg_ + offset, regScaled_);
    }

    RegisterOffsetRegisterScaled RegisterRegisterScaled::operator-(int64_t offset) const {
        return RegisterOffsetRegisterScaled(reg_ - offset, regScaled_);
    }

    RegisterOffsetRegisterScaled RegisterScaledOffset::operator+(Register reg) const {
        return RegisterOffsetRegisterScaled(reg + offset_, regScaled_);
    }
}
