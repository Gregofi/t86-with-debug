#pragma once

#include "instruction.h"
#include <cstdint>
#include <iostream>
#include <vector>

namespace tiny::t86 {

/**
 * Simple wrapper for vector of instructions
 */
struct Program {
    Program(std::vector<std::unique_ptr<Instruction>> instructions = {},
        std::vector<int64_t> data = {})
        : instructions_(std::move(instructions))
        , data_(std::move(data)) { }

    Program(Program&& other)
        : instructions_(std::move(other.instructions_))
        , data_(std::move(other.data_)) { }

    ~Program() { }

    Program& operator=(Program&& other) {
        instructions_ = std::move(other.instructions_);
        data_ = std::move(other.data_);
        return *this;
    }

    const Instruction& at(size_t index) const;

    const std::vector<int64_t>& data() const { return data_; }

    const std::vector<std::unique_ptr<Instruction>>& instructions() const {
        return instructions_;
    }

    std::vector<std::unique_ptr<Instruction>> moveInstructions() {
        return std::move(instructions_);
    }

    void dump() const {
        unsigned cnt = 0;
        for (const auto& ins : instructions_) {
            std::cerr << cnt++ << ": " << ins->toString() << "\n";
        }
    }

    std::vector<std::unique_ptr<Instruction>> instructions_;

    std::vector<int64_t> data_;
};
} // namespace tiny::t86
