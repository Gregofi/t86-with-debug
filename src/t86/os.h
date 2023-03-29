#pragma once

#include "t86/cpu.h"
#include "t86/debug.h"
#include "t86/program.h"
#include "common/logger.h"

namespace tiny::t86 {
class OS {
public:
    OS(size_t register_count = 8, size_t float_register_count = 4, size_t memory_size = 1024): cpu(register_count, float_register_count, memory_size) {}

    void Run(Program program);

    void SetDebuggerComms(std::unique_ptr<Messenger> m) {
        debug_interface.emplace(cpu, std::move(m));
    }
private:
    void DebuggerMessage(Debug::BreakReason reason);
    void DispatchInterrupt(int n);
    /// If debug interface is present then sends message to it,
    /// otherwise noop.
    /// If returns false then the execution should be aborted.
    Cpu cpu;
    std::optional<Debug> debug_interface;
    /// Indicates whether the execution should stop.
    bool stop{false};
};
}
