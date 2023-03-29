#pragma once

#include <optional>
#include "../../instruction.h"
#include "../branchpredictor.h"

namespace tiny::t86 {
    class NaiveBranchPredictor : public BranchPredictor {
    public:
        uint64_t nextGuess(uint64_t pc, const JumpInstruction& instruction) const override;

        void registerBranchTaken(uint64_t pc, uint64_t destination) override;

        void registerBranchNotTaken(uint64_t pc) override;
    };
}
