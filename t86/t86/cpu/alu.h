#pragma once

#include <cstdint>

namespace tiny::t86::Alu {
    struct Flags {

        Flags(int64_t value = 0);

        Flags(bool sign, bool zero, bool carry, bool overflow);

        operator int64_t() const;

        bool signFlag;

        bool zeroFlag;

        bool carryFlag;

        bool overflowFlag;
    };

    struct Result {
        int64_t value = 0;
        Flags flags;
    };

    struct FloatResult {
        double value;
        Flags flags;
    };

    Result add(int64_t x, int64_t y);

    Result subtract(int64_t x, int64_t y);

    Result negate(int64_t x);

    Result multiply(int64_t x, int64_t y);

    Result divide(int64_t x, int64_t y);

    Result signed_multiply(int64_t x, int64_t y);

    Result signed_divide(int64_t x, int64_t y);

    Result bit_and(int64_t x, int64_t y);

    Result bit_or(int64_t x, int64_t y);

    Result bit_xor(int64_t x, int64_t y);
    
    Result bit_not(int64_t x);

    Result bit_left_shift(int64_t x, int64_t y);

    Result bit_right_shift(int64_t x, int64_t y);

    // Result bit_left_roll(int64_t x, int64_t y);

    // Result bit_right_roll(int64_t x, int64_t y);

    // TODO remove after testing
    Result mod(int64_t x, int64_t y);

    // Float

    FloatResult fadd(double x, double y);

    FloatResult fsubtract(double x, double y);

    FloatResult fmultiply(double x, double y);

    FloatResult fdivide(double x, double y);
}
