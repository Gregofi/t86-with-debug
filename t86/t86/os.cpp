#include <cassert>
#include <exception>
#include <fmt/core.h>

#include "common/logger.h"
#include "debug.h"
#include "os.h"

namespace tiny::t86 {
void OS::DispatchInterrupt(int n) {
    switch (n) {
    case 3:
        DebuggerMessage(Debug::BreakReason::SoftwareBreakpoint);
        break;
    case 1:
        DebuggerMessage(Debug::BreakReason::SingleStep);
        break;
    default:
        // TODO: Add logging!
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
    }
}
} // namespace tiny::t86
