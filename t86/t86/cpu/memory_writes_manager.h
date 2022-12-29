#pragma once

#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "memory_writes_manager/memory_write.h"
#include "memory_writes_manager/memory_writes.h"

namespace tiny::t86 {
    class MemoryWritesManager {
    public:
        MemoryWrite::Id currentMaxWriteId() const {
            return currentId;
        }

        /// Removes all finished outgoing writes
        void removeFinished(const RAM& ram);

        /**
         * Removes all pending writes
         * Used when undoing speculation
         */
        void removePending();

        /// Register future write, we don't know the address right now
        MemoryWrite::Id registerPendingWrite();

        /// Register future write, with specific address
        MemoryWrite::Id registerPendingWrite(std::size_t address);

        /// Specify address to previously registered write without address
        void specifyAddress(MemoryWrite::Id, std::size_t address);

        /// Specify value of the write, this does not transitions the write to outgoing state
        void specifyValue(MemoryWrite::Id, uint64_t value) const;

        /// Starts the writing
        void startWriting(MemoryWrite::Id id, RAM& ram);

        bool hasUnspecifiedWrites(MemoryWrite::Id maxId) const {
            auto it = unspecifiedWrites_.lower_bound(maxId);
            return it != unspecifiedWrites_.end();
        }

        /**
         * Checks if there are some pending writes
         * It DOES NOT take into account all writes with unspecified address
         * Check hasUnspecifiedWrites
         */
        std::optional<MemoryWrite> previousWrite(std::size_t address, MemoryWrite::Id maxId) const;

        MemoryWrite& getWrite(MemoryWrite::Id id) const;

    private:
        MemoryWrite::Id currentId{0};

        std::unordered_map<std::size_t, MemoryWrites> writesMap_;

        std::unordered_map<MemoryWrite::Id, MemoryWrite&> writesById;

        std::set<MemoryWrite::Id, std::greater<>> unspecifiedWrites_;
    };
}
