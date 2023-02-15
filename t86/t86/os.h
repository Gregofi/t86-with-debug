#pragma once

#include "t86/cpu.h"
#include "t86/debug.h"
#include "t86/program.h"

namespace tiny::t86 {
class OS {
public:
    OS(int debugger_port): debug_interface(std::in_place, debugger_port, cpu) {

    }

    OS() = default;

    void Run(Program program);

private:
    void DispatchInterrupt(int n);

    Cpu cpu;
    std::optional<Debug> debug_interface;
};
}
