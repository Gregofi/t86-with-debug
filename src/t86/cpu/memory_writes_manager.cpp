#include <cassert>

#include "memory_writes_manager.h"

namespace tiny::t86 {

    MemoryWrite::Id MemoryWritesManager::registerPendingWrite() {
        MemoryWrite::Id writeId = ++currentId;
        unspecifiedWrites_.insert(writeId);
        return writeId;
    }

    MemoryWrite::Id MemoryWritesManager::registerPendingWrite(std::size_t address) {
        MemoryWrite::Id writeId = ++currentId;
        writesById.emplace(writeId, writesMap_[address].add(writeId, address));
        return writeId;
    }

    void MemoryWritesManager::specifyAddress(MemoryWrite::Id id, std::size_t address) {
        std::size_t erased = unspecifiedWrites_.erase(id);
        assert(erased == 1
                && "Trying to specify address for invalid, unknown or already specified write id");
        writesById.emplace(id, writesMap_[address].add(id, address));
    }

    void MemoryWritesManager::specifyValue(MemoryWrite::Id id, uint64_t value) const {
        auto& write = getWrite(id);
        write.setValue(value);
    }

    std::optional<MemoryWrite> MemoryWritesManager::previousWrite(std::size_t address, MemoryWrite::Id maxId) const {
        assert(!hasUnspecifiedWrites(maxId));
        // Lets check, if there are some pending writes to this address
        auto it = writesMap_.find(address);
        if (it == writesMap_.end()) {
            return std::nullopt;
        }
        return it->second.latest(maxId);
    }

    void MemoryWritesManager::removeFinished(const RAM& ram) {
        for (auto& [address, writes] : writesMap_) {
            auto removedIds = writes.removeFinished(ram);
            for (MemoryWrite::Id id : removedIds) {
                writesById.erase(id);
            }
        }
    }

    void MemoryWritesManager::removePending() {
        for (auto& [address, writes] : writesMap_) {
            auto removedIds = writes.removePending();
            for (MemoryWrite::Id id : removedIds) {
                writesById.erase(id);
            }
        }
        unspecifiedWrites_.clear();
    }

    MemoryWrite& MemoryWritesManager::getWrite(MemoryWrite::Id id) const {
        auto it = writesById.find(id);
        assert(it != writesById.end() && "Unknown id");
        return it->second;
    }

    void MemoryWritesManager::startWriting(MemoryWrite::Id id, RAM& ram) {
        MemoryWrite& write = getWrite(id);
        assert(write.isPending() && write.hasValue());
        write.setWriteId(ram.write(write.address(), write.value()));
    }
}
