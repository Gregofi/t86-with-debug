#pragma once

#include "../../ram.h"

#include <utility>
#include <cstdint>
#include <optional>

namespace tiny::t86 {
    class MemoryWrite {
    public:
        using Id = std::size_t;

        MemoryWrite(Id id, std::size_t address)
        : id_(id), address_(address)
        {}

        Id id() const {
            return id_;
        }

        std::size_t address() const {
            return address_;
        }

        bool operator<(const MemoryWrite& other) const {
            return id_ > other.id_;
        }

        bool isPending() const {
            return !ramWriteId_.has_value();
        }

        bool isOutgoing() const {
            return ramWriteId_.has_value();
        }

        void setWriteId(RAM::WriteId);

        RAM::WriteId writeId() const {
            return ramWriteId_.value();
        }

        bool hasValue() const {
            return value_.has_value();
        }

        void setValue(uint64_t value);

        uint64_t value() const {
            return value_.value();
        }
    private:
        Id id_;

        std::size_t address_;

        std::optional<uint64_t> value_;

        std::optional<RAM::WriteId> ramWriteId_;
    };
}
