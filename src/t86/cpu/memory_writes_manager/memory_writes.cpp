#include "memory_writes.h"

#include <cassert>
#include <algorithm>

namespace tiny::t86 {
    std::vector<MemoryWrite>::iterator MemoryWrites::find(MemoryWrite::Id id) {
        return std::lower_bound(writes_.begin(), writes_.end(), id, comp);
    }

    std::vector<MemoryWrite>::const_iterator MemoryWrites::find(MemoryWrite::Id id) const {
        return std::lower_bound(writes_.begin(), writes_.end(), id, comp);
    }

    std::optional<MemoryWrite> MemoryWrites::latest(MemoryWrite::Id maxId) const {
        auto it = find(maxId);
        if (it == writes_.end()) {
            return std::nullopt;
        }
        return *it;
    }

    MemoryWrite& MemoryWrites::add(MemoryWrite::Id id, std::size_t address) {
        auto it = find(id);
        return *writes_.emplace(it, MemoryWrite(id, address));
    }

    std::vector<MemoryWrite::Id> MemoryWrites::removeFinished(const RAM& ram) {
        std::vector<MemoryWrite::Id> removed;
        for (auto writeIt = writes_.begin(); writeIt != writes_.end();) {
            if (writeIt->isOutgoing() && !ram.pending(writeIt->writeId())) {
                // Already finished
                removed.push_back(writeIt->id());
                writeIt = writes_.erase(writeIt);
            } else {
                // Either still pending or not finished writing
                ++writeIt;
            }
        }
        return removed;
    }

    std::vector<MemoryWrite::Id> MemoryWrites::removePending() {
        std::vector<MemoryWrite::Id> removed;
        for (auto writeIt = writes_.begin(); writeIt != writes_.end();) {
            if (writeIt->isPending()) {
                // Still pending
                removed.push_back(writeIt->id());
                writeIt = writes_.erase(writeIt);
            } else {
                // Outgoing write
                ++writeIt;
            }
        }
        return removed;
    }


}