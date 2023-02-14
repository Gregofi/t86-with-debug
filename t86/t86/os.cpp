#include <cassert>

#include "os.h"
#include "debug.h"

namespace tiny::t86 {
void OS::DispatchInterrupt(int n) {
    switch (n) {
    case 3:
        debug_interface->Work(Debug::BreakReason::SoftwareBreakpoint);
        break;
    default:
        // TODO: Add logging!
        assert(false); 
        break;
    }
}

void OS::Run(Program program) {
    cpu.start(std::move(program));
    while (true) {
        cpu.tick();
        if (cpu.halted()) {
            return;
        }

        if (int n = cpu.interrupted(); n > 0) {
            DispatchInterrupt(n);
        }
    }
}
}
