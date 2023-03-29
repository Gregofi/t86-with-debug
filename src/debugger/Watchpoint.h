#pragma once
#include <cstddef>


struct Watchpoint {
    enum class Type {
        Read, // Not implemented
        Write,
    } type;
    // Which hardware register the WP occupies.
    size_t hw_reg;
};
