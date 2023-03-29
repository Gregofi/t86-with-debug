#pragma once

#include "../instruction.h"

#include <cstddef>

namespace tiny::t86 {
    class BranchPredictor {
    public:
        virtual ~BranchPredictor() = default;

        // Instruction pointer would be unique
        // but pc is provided so that the predictor can predict some relative jumps
        // Returns new pc
        virtual uint64_t nextGuess(uint64_t pc, const JumpInstruction& instruction) const = 0;

        virtual void registerBranchTaken(uint64_t pc, uint64_t destination) = 0;

        virtual void registerBranchNotTaken(uint64_t pc) = 0;
    };
}
