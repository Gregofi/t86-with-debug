#pragma once

#include "cpu.h"
#include "debug.h"
#include "program.h"

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
