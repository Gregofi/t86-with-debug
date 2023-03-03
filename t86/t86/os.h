#pragma once

#include "common/logger.h"
#include "t86/cpu.h"
#include "t86/debug.h"
#include "t86/program.h"

namespace tiny::t86 {
class OS {
public:
    OS() = default;
    OS(size_t register_count)
        : cpu(register_count) { }
    OS(size_t register_count, std::unique_ptr<Messenger> messenger)
        : cpu(register_count)
        , debug_interface(std::in_place, cpu, std::move(messenger)) { }

    void Run(Program program);

    void SetDebuggerComms(std::unique_ptr<Messenger> m) {
        debug_interface.emplace(cpu, std::move(m));
    }

private:
    void DispatchInterrupt(int n);
    /// If debug interface is present then sends message to it,
    /// otherwise noop.
    void DebuggerMessage(Debug::BreakReason reason) {
        if (debug_interface) {
            debug_interface->Work(reason);
        }
    }

    Cpu cpu;
    std::optional<Debug> debug_interface;
};
} // namespace tiny::t86
