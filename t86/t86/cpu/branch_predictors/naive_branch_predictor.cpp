#include "naive_branch_predictor.h"

uint64_t tiny::t86::NaiveBranchPredictor::nextGuess(uint64_t pc, const JumpInstruction& instruction) const {
    const Operand& destination = instruction.getDestination();
    if (destination.isFetched()) {
        return destination.getValue();
    }
    else {
        // Just continue, without any special treatment
        return ++pc;
    }
}

void tiny::t86::NaiveBranchPredictor::registerBranchTaken(uint64_t, uint64_t) {}

void tiny::t86::NaiveBranchPredictor::registerBranchNotTaken(uint64_t) {}
