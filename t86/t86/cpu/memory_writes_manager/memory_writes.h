#pragma once

#include <utility>
#include <optional>
#include <set>

#include "../../ram.h"
#include "memory_write.h"

namespace tiny::t86 {
    class MemoryWrites {
    public:
        /**
         * Adds to the collection or memory writes
         * Creates pending write without any value
         */
        MemoryWrite& add(MemoryWrite::Id id, std::size_t address);

        /**
         * Returns optional element of either same or lesser id (using comp function)
         */
        std::optional<MemoryWrite> latest(MemoryWrite::Id maxId) const;

        /**
         * Checks all outgoing writes
         * if they are finished removes them
         * @return vector of ids of removed writes
         */
        std::vector<MemoryWrite::Id> removeFinished(const RAM& ram);

        /**
         * Removes all pending writes
         * used when undoing speculation
         * @return vector of ids of removed writes
         */
        std::vector<MemoryWrite::Id> removePending();

    private:
        // This is used to sort and lookup in the writes
        // It is sorting from largest to smallest to find writes with equal or lesser id
        // using std::lower_bound
        static bool comp(const MemoryWrite& e, MemoryWrite::Id i) {
            return e.id() > i;
        }

        // This vector is sorted using comp function, so that we can find writes
        // with id up to some value
        // List would have slow lookup times, but better insertions
        // Set does not allow modifications of it's elements once inserted
        // Might change to list in the future after some benchmarking
        std::vector<MemoryWrite> writes_;

        std::vector<MemoryWrite>::iterator find(MemoryWrite::Id id);
        std::vector<MemoryWrite>::const_iterator find(MemoryWrite::Id id) const;
    };
}
