#include <cassert>
#include <exception>
#include <fmt/core.h>

#include "os.h"
#include "debug.h"
#include "common/logger.h"

namespace tiny::t86 {
void OS::DispatchInterrupt(int n) {
    switch (n) {
    case 3:
        DebuggerMessage(Debug::BreakReason::SoftwareBreakpoint);
        break;
    case 2:
        DebuggerMessage(Debug::BreakReason::HardwareBreakpoint);
        break;
    case 1:
        DebuggerMessage(Debug::BreakReason::SingleStep);
        break;
    default:
        throw std::runtime_error(
              fmt::format("No interrupt handler for interrupt no. {}!", n));
        break;
    }
}

void OS::Run(Program program) {
    cpu.start(std::move(program));
    DebuggerMessage(Debug::BreakReason::Begin);
    log_info("Starting execution\n");
    while (true) {
        cpu.tick();
        if (cpu.halted()) {
            log_info("Halt");
            DebuggerMessage(Debug::BreakReason::Halt);
            return;
        }

        if (int n = cpu.interrupted(); n > 0) {
            log_info("Interrupt {} occurred", n);
            DispatchInterrupt(n);
        }

        if (stop) {
            log_info("OS: stop is set, ending");
            return;
        }
    }
}

void OS::DebuggerMessage(Debug::BreakReason reason) {
    if (debug_interface) {
        stop = !debug_interface->Work(reason);
    } else {
        log_warning("Call to debugger interface was initiated but no "
                    "debugger is connected!");
    }
}
}
