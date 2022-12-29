#include "program.h"
#include "instruction.h"

namespace tiny::t86 {
    const Instruction* Program::at(size_t index) const {
        if (index >= instructions_.size()) {
            static NOP nop;
            return &nop;
        }
        return instructions_.at(index);
    }

    void Program::deleteInstructions() {
        for (Instruction* ins : instructions_) {
            delete ins;
        }
    }
}