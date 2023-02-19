#include <cassert>

#include "os.h"
#include "debug.h"
#include "common/logger.h"

namespace tiny::t86 {
void OS::DispatchInterrupt(int n) {
    switch (n) {
    case 3:
        DebuggerMessage(Debug::BreakReason::SoftwareBreakpoint);
        break;
    default:
        // TODO: Add logging!
        log_error("No interrupt handler for interrupt no. {}!", n);
        assert(false); 
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
}
