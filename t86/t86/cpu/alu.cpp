#include "alu.h"

#include <limits>
#include <cmath>

namespace tiny::t86::Alu {
    Flags::operator int64_t() const {
        return (signFlag << 0) | (zeroFlag << 1) | (carryFlag << 2) | (overflowFlag << 3);
    }

    Flags::Flags(int64_t value) {
        signFlag = value & 0x1;
        zeroFlag = value & 0x2;
        carryFlag = value & 0x4;
        overflowFlag = value & 0x8;
    }

    Flags::Flags(bool sign, bool zero, bool carry, bool overflow)
            : signFlag(sign), zeroFlag(zero), carryFlag(carry), overflowFlag(overflow) {}

    Result add(int64_t x, int64_t y) {
        int64_t result = x + y;
        uint64_t ux = static_cast<uint64_t>(x);
        uint64_t uy = static_cast<uint64_t>(y);
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool carryFlag = ux > (std::numeric_limits<uint64_t>::max() - uy);
        bool overflowFlag = (y > 0 && x > (std::numeric_limits<int64_t>::max() - y))
                            || (y < 0 && x < (std::numeric_limits<int64_t>::min() - y));
        return Result{result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag}};
    }

    Result subtract(int64_t x, int64_t y) {
        int64_t result = x - y;
        uint64_t ux = static_cast<uint64_t>(x);
        uint64_t uy = static_cast<uint64_t>(y);
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool carryFlag = ux < uy;
        bool overflowFlag = (y > 0 && x < (std::numeric_limits<int64_t>::min() + y))
                            || (y < 0 && x > (std::numeric_limits<int64_t>::max() + y));
        return Result{result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag}};
    }

    Result negate(int64_t x) {
        int64_t result = -x;
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool overflowFlag = false;
        bool carryFlag = false;
        return Result{result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag}};
    }

    Result multiply(int64_t x, int64_t y) {
        uint64_t ux = static_cast<uint64_t>(x);
        uint64_t uy = static_cast<uint64_t>(y);
        uint64_t uresult = ux * uy;
        int64_t result = static_cast<int64_t>(uresult);
        bool signFlag = result < 0;
        bool zeroFlag = uresult == 0;
        bool carryFlag = x != 0 && y != 0 && ux != uresult / uy;
        bool overflowFlag = false;
        return Result{result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag}};
    }

    Result divide(int64_t x, int64_t y) {
        uint64_t ux = static_cast<uint64_t>(x);
        uint64_t uy = static_cast<uint64_t>(y);
        uint64_t uresult = ux / uy;
        int64_t result = static_cast<int64_t>(uresult);
        bool signFlag = result < 0;
        bool zeroFlag = uresult == 0;
        bool overflowFlag = false;
        bool carryFlag = false;
        return Result{result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag}};
    }

    Result signed_multiply(int64_t x, int64_t y) {
        int64_t result = x * y;
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool carryFlag = false;
        bool overflowFlag = x != 0 && y != 0 && x != result / y;
        return Result{result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag}};
    }

    Result signed_divide(int64_t x, int64_t y) {
        int64_t result = x / y;
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool overflowFlag = false;
        bool carryFlag = false;
        return Result{result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag}};
    }

    Result bit_and(int64_t x, int64_t y) {
        int64_t result = x & y;
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool overflowFlag = false;
        bool carryFlag = false;
        return Result{result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag}};
    }

    Result bit_or(int64_t x, int64_t y) {
        int64_t result = x | y;
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool overflowFlag = false;
        bool carryFlag = false;
        return Result{result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag}};
    }

    Result bit_xor(int64_t x, int64_t y) {
        int64_t result = x ^ y;
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool overflowFlag = false;
        bool carryFlag = false;
        return Result{result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag}};
    }

    Result bit_not(int64_t x) {
        int64_t result = ~x;
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool overflowFlag = false;
        bool carryFlag = false;
        return Result{ result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag} };
    }

    Result bit_left_shift(int64_t x, int64_t y) {
        int64_t result = x << y;
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool overflowFlag = false;
        bool carryFlag = false;
        if (y <= 64) {
            // the most significant bit before the last shift
            carryFlag = (x << (y - 1)) < 0;
        }
        return Result{result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag}};
    }

    Result bit_right_shift(int64_t x, int64_t y) {
        int64_t result = x >> y;
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool overflowFlag = false;
        bool carryFlag = false;
        if (y <= 64) {
            // the most significant bit before the last shift
            carryFlag = ((x >> (y - 1)) & 1) != 0;
        }
        return Result{result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag}};
    }

    /*
    Result bit_left_roll(int64_t x, int64_t y) {
        // TODO
        // We can roll only 64-1 times
        int64_t roll = y & 63;
        // Set higher bits by shifting
        int64_t result = x << roll
        // Set lower bits by taking the top y bits
        return Result();
    }

    Result bit_right_roll(int64_t x, int64_t y) {
        // TODO
        return Result();
    }
    */
    
    // TODO Remove after testing
    Result mod(int64_t x, int64_t y) {
        int64_t result = x % y;
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool carryFlag = false;
        bool overflowFlag = false;
        return Result{result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag}};
    }

    FloatResult fadd(double x, double y) {
        double result = x + y;
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool carryFlag = false;
        bool overflowFlag = std::isinf(result);
        return FloatResult{result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag}};
    }
    FloatResult fsubtract(double x, double y) {
        double result = x - y;
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool carryFlag = false;
        bool overflowFlag = std::isinf(result);
        return FloatResult{ result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag} };
    }
    FloatResult fmultiply(double x, double y) {
        double result = x * y;
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool carryFlag = false;
        bool overflowFlag = std::isinf(result);
        return FloatResult{ result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag} };
    }
    FloatResult fdivide(double x, double y) {
        double result = x / y;
        bool signFlag = result < 0;
        bool zeroFlag = result == 0;
        bool carryFlag = false;
        bool overflowFlag = std::isinf(result);
        return FloatResult{ result, Flags{signFlag, zeroFlag, carryFlag, overflowFlag} };
    }
}
