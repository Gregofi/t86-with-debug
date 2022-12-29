#pragma once

#include <array>
#include <optional>
#include <list>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <functional>

namespace tiny::t86 {
    class RAM {
    public:
        using WriteId = size_t;

        RAM(std::size_t memSize, std::size_t gatesCnt);

        void tick();

        std::size_t readLatency(std::size_t address) const;

        std::size_t writeLatency(std::size_t address) const;

        std::optional<int64_t> read(std::size_t address);

        WriteId write(std::size_t address, int64_t value);

        bool isBusy() const;

        std::size_t size() const;

        bool pending(WriteId id) const;

    public: /// These functions should be used only for debug purposes
        int64_t get(std::size_t address) const;

        void set(std::size_t address, int64_t value);

    private:
        WriteId writeIdCounter {0};

        // TODO changeable mem size
        std::vector<int64_t> mem_;
        // TODO changeable gates count
        std::size_t gatesCnt_;

        struct ReadEntry {
            std::size_t remainingCnt;
            int64_t value;
        };

        struct WriteEntry {
            WriteId id;
            std::size_t remainingCnt;
            int64_t value;
        };

        std::unordered_map<size_t, ReadEntry> reads_;

        std::unordered_map<size_t, WriteEntry> writes_;

        // Helper lookup map by id
        std::unordered_map<WriteId, std::reference_wrapper<const WriteEntry>> writesById_;
    };
}
