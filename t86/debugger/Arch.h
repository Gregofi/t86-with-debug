#pragma once
#include <string>
#include <map>

class Arch {
    /// Default machine is T86.
    Arch() {
        current_machine = Machine::T86;
    }
public:
    enum class Machine {
        T86,
    };

    static void SetArch(Machine arch) {
        auto &ins = GetInstance();
        ins.current_machine = arch;
    }

    static bool SupportHardwareLevelSingleStep() {
        auto &ins = GetInstance();
        switch (ins.current_machine) {
        case Machine::T86: return true;
        }
    }

    static std::string GetSoftwareBreakpointOpcode() {
        auto &ins = GetInstance();
        static const std::map<Arch::Machine, std::string> opcode_map = {
            {Arch::Machine::T86, "BKPT"},
        };
        return opcode_map.at(ins.GetMachine());
    }

    static Machine GetMachine() {
        auto &ins = GetInstance();
        return ins.current_machine;
    }

    Arch(const Arch&) = delete;
    Arch operator=(Arch) = delete;
private:
    static Arch& GetInstance() {
        static Arch inst;
        return inst;
    }

    Machine current_machine;
};
