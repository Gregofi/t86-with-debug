#pragma once

#include <string>
#include <utility>
#include <limits>

namespace tiny::t86 {
    /**
     * Represents instruction labels in asm
     */
    class Label {
    public:
        static Label empty() {
            return Label(std::numeric_limits<uint64_t>::max());
        }

        Label(uint64_t address) : address_(address) {}

        uint64_t address() const {
            return address_;
        }

        operator uint64_t() const {
            return address_;
        }

    private:
        uint64_t address_;
    };

    /**
     * Represents data label in data segment
     */
    class DataLabel {
    public:
        DataLabel(uint64_t address) : address_(address) {}

        operator int64_t() const {
            return address_;
        }
    private:
        uint64_t address_;
    };
}
