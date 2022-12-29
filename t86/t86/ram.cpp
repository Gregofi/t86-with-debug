#include <cassert>

#include "ram.h"

namespace tiny::t86 {

    RAM::RAM(std::size_t memSize, std::size_t gatesCnt) : mem_(memSize, 0), gatesCnt_(gatesCnt) {}

    void RAM::tick() {
        // writes and reads "linger" around for one tick after being finished
        for (auto writeIt = writes_.begin(); writeIt != writes_.end();) {
            auto& write = writeIt->second;
            if (write.remainingCnt == 0) {
                // Remove from the helper lookup map
                writesById_.erase(write.id);
                // Remove from this map
                writeIt = writes_.erase(writeIt);
            } else {
                --write.remainingCnt;
                ++writeIt;
            }
        }

        for (auto readIt = reads_.begin(); readIt != reads_.end();) {
            auto& read = readIt->second;
            if (read.remainingCnt == 0) {
                readIt = reads_.erase(readIt);
            } else {
                --read.remainingCnt;
                ++readIt;
            }
        }
    }

    std::optional<int64_t> RAM::read(std::size_t address) {
        // Check reads
        if (auto it = reads_.find(address); it != reads_.end()) {
            const auto& read = it->second;
            if (read.remainingCnt == 0) {
                // Ready
                return read.value;
            } else {
                // Waiting to be fetched
                return std::nullopt;
            }
        }

        if (!isBusy()) {
            // Start reading
            assert(reads_.find(address) == reads_.end());
            assert(writes_.find(address) == writes_.end() && "You should not read from address that is being written to");
            reads_[address] = ReadEntry{readLatency(address), mem_.at(address)};
        }

        return std::nullopt;
    }

    bool RAM::isBusy() const {
        return reads_.size() == gatesCnt_;
    }

    RAM::WriteId RAM::write(std::size_t address, int64_t value) {
        mem_.at(address) = value;
        WriteId id = writeIdCounter++;
        if (auto it = writes_.find(address); it != writes_.end()) {
            writesById_.erase(it->second.id);
        }
        writesById_.insert(std::make_pair(id, std::cref(writes_[address] = WriteEntry{id, writeLatency(address), value})));
        return id;
    }

    int64_t RAM::get(std::size_t address) const {
        return mem_.at(address);
    }

    void RAM::set(std::size_t address, int64_t value) {
        mem_.at(address) = value;
    }

    std::size_t RAM::size() const {
        return mem_.size();
    }

    bool RAM::pending(RAM::WriteId id) const {
        return writesById_.find(id) != writesById_.end();
    }
}
