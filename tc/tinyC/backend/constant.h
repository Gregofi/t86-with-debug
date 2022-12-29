#pragma once

#include "tinyC/backend/value.h"
#include "common/helpers.h"

#include "operand.h"
#include "opcode.h"

#include <unordered_map>
#include <memory>


namespace tinyc {
    /**
     * Represents values that are constants, like integers, doubles, const chars and so on.
     */
    class Constant: public Value {
    public:
        /**
         * Generates according mov instruction corresponding to this value type.
         * Saves the value into register which corresponds to this class ID.
         *
         * TODO: This is not ideal, since the IR shouldn't know anything about the target.
         * @return
         */
        virtual T86Ins generateMov(Register) const = 0;

        void accept([[maybe_unused]] IRVisitor& v) override {
            UNREACHABLE;
        };
    };

    /**
     * An 8byte integer value
     */
    class ConstantInt: public Constant {
        int64_t value;

        explicit ConstantInt(int64_t value) : value(value) {}

    public:
        static std::unordered_map<int64_t, ConstantInt*>& constants();

        static ConstantInt* get(int64_t val);

        int64_t getVal() const { return value; }

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override;

        std::string toStringID() const override;

        T86Ins generateMov(Register reg) const override;
    };

    /**
     * An 8byte floating point value.
     */
    class ConstantDouble: public Constant {
        double value;
    public:
        explicit ConstantDouble(double value) : value(value) {}
    };
}
