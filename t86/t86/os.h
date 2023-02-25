#pragma once

#include "t86/cpu.h"
#include "t86/debug.h"
#include "t86/program.h"
#include "common/logger.h"

namespace tiny::t86 {
class OS {
public:
    OS() = default;
    OS(std::unique_ptr<Messenger> messenger): debug_interface(std::in_place, cpu, std::move(messenger)) {
    }

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
}
