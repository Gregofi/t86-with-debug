#pragma once

#include "t86/cpu.h"
#include "t86/debug.h"
#include "t86/program.h"
#include "common/logger.h"

namespace tiny::t86 {
class OS {
public:
    OS() = default;
    OS(size_t register_count, size_t float_register_count): cpu(register_count, float_register_count) {}
    OS(size_t register_count, size_t float_register_count, std::unique_ptr<Messenger> messenger): cpu(register_count, float_register_count), debug_interface(std::in_place, cpu, std::move(messenger)) {
    }

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
