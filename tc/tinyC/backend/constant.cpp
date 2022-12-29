#include "tinyC/backend/constant.h"
#include "operand.h"
#include "opcode.h"
#include <unordered_map>
#include <memory>

namespace tinyc {
    std::unordered_map<int64_t, ConstantInt*>& tinyc::ConstantInt::constants() {
        static std::unordered_map<int64_t, ConstantInt*> constants;
        return constants;
    }

    ConstantInt* ConstantInt::get(int64_t val) {
        auto& cts = constants();
        if (cts.contains(val)) {
            return cts[val];
        }
        return cts[val] = new ConstantInt(val);
    }

    colors::ColorPrinter& ConstantInt::print(colors::ColorPrinter& printer) {
        // For some reason, the 'value' sets the color to red.
        return printer << "int:" << value << colors::color::reset;
    }

    std::string ConstantInt::toStringID() const {
        return std::to_string(value);
    }

    T86Ins ConstantInt::generateMov(Register reg) const {
        return T86Ins{T86Ins::Opcode::MOV, {reg, Immediate{static_cast<int>(value)}}};
    }
}
