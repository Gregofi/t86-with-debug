#pragma once

#include "t86/cpu.h"
#include "t86/debug.h"
#include "t86/program.h"
#include "common/logger.h"

namespace tiny::t86 {
class OS {
public:
    OS() = default;

    void Run(Program program);

    /// Litens for incoming debugger connections, blocking call.
    void ListenForDebugger(int port) {
        if (debug_interface) {
            log_info("Debugging interface is already in place!");
            return;
        }
        debug_interface.emplace(port, cpu);
        debug_interface->OpenConnection();
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
