#include "memory_write.h"

#include <cassert>

namespace tiny::t86 {
    void MemoryWrite::setWriteId(RAM::WriteId writeId) {
        assert(isPending() && !isOutgoing());
        ramWriteId_ = writeId;
    }

    void MemoryWrite::setValue(uint64_t value) {
        assert(!hasValue() && !isOutgoing());
        value_ = value;
    }
}
