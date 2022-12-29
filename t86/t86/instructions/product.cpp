#include "product.h"

#include <cassert>

namespace tiny::t86 {
    Product Product::fromOperand(Operand op) {
        if (op.isRegister()) {
            return op.getRegister();
        } else if (op.isMemoryImmediate()) {
            return op.getMemoryImmediate();
        } else if (op.isMemoryRegister()
                || op.isMemoryRegisterOffset()
                || op.isMemoryRegisterScaled()
                || op.isMemoryRegisterRegister()
                || op.isMemoryRegisterOffsetRegister()
                || op.isMemoryRegisterRegisterScaled()
                || op.isMemoryRegisterOffsetRegisterScaled()) {
            return MemoryRegister();
        } else if (op.isFloatRegister()) {
            return op.getFloatRegister();
        } else if (op.isRegisterOffset()
                || op.isRegisterScaled()
                || op.isRegisterRegister()
                || op.isRegisterOffsetRegister()
                || op.isRegisterRegisterScaled()
                || op.isRegisterOffsetRegisterScaled()) {
            throw std::runtime_error("Trying to convert operand to product, that can be only used as value");
        }
        throw std::runtime_error("Unknown operand type");
    }

    bool Product::isRegister() const {
        return std::holds_alternative<Register>(product_);
    }

    Register Product::getRegister() const {
        assert(isRegister());
        return std::get<Register>(product_);
    }

    bool Product::isFloatRegister() const {
        return std::holds_alternative<FloatRegister>(product_);
    }

    FloatRegister Product::getFloatRegister() const {
        assert(isFloatRegister());
        return std::get<FloatRegister>(product_);
    }

    bool Product::isMemoryImmediate() const {
        return std::holds_alternative<Memory::Immediate>(product_);
    }

    Memory::Immediate Product::getMemoryImmediate() const {
        assert(isMemoryImmediate());
        return std::get<Memory::Immediate>(product_);
    }

    bool Product::isMemoryRegister() const {
        return std::holds_alternative<MemoryRegister>(product_);
    }
}
