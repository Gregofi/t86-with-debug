#pragma once
#include "debugger/DebuggerError.h"
#include "fmt/core.h"
#include "helpers.h"
#include <cassert>
#include <cmath>
#include <set>
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
        UNREACHABLE;
    }

    static std::string GetSoftwareBreakpointOpcode() {
        auto &ins = GetInstance();
        static const std::map<Arch::Machine, std::string> opcode_map = {
            {Arch::Machine::T86, "BKPT"},
        };
        return opcode_map.at(ins.GetMachine());
    }

    static size_t DebugRegistersCount() {
        assert(SupportsHardwareWatchpoints());
        auto &ins = GetInstance();
        if (ins.current_machine == Machine::T86) {
            return 4;
        } else {
            UNREACHABLE;
        }
    }

    static bool SupportsHardwareWatchpoints() {
        auto &ins = GetInstance();
        return ins.current_machine == Machine::T86;
    }

    static Machine GetMachine() {
        auto &ins = GetInstance();
        return ins.current_machine;
    }

    static void SetDebugRegister(size_t idx, uint64_t address,
                                 std::map<std::string, uint64_t>& regs) {
        auto &ins = GetInstance();

        if (idx >= Arch::DebugRegistersCount()) {
            throw DebuggerError("Out of bounds: Debug registers");
        }

        if (ins.current_machine == Machine::T86) {
            regs.at(fmt::format("D{}", idx)) = address;
        } else {
            UNREACHABLE;
        }
    }

    static void ActivateDebugRegister(size_t idx, std::map<std::string, uint64_t>& regs) {
        auto &ins = GetInstance();

        if (ins.current_machine == Machine::T86) {
            // D4 is control register
            auto& control_reg = regs.at("D4");
            // First fourth bits indicate whether the register is active.
            control_reg |= 1 << idx;
        } else {
            UNREACHABLE;
        }
    }

    static void DeactivateDebugRegister(size_t idx,
                                        std::map<std::string, uint64_t>& regs) {
        auto &ins = GetInstance();

        if (ins.current_machine == Machine::T86) {
            auto& control_reg = regs.at("D4");
            // First fourth bits indicate whether the register is active.
            control_reg &= ~(1 << idx);
        } else {
            UNREACHABLE;
        }
    }

    /// Return index of debug register that caused a trap to occur.
    static size_t GetResponsibleRegister(const std::map<std::string, uint64_t>& regs) {
        assert(SupportsHardwareWatchpoints());
        auto &ins = GetInstance();
        if (ins.current_machine == Machine::T86) {
            auto& control_reg = regs.at("D4");
            auto idx_masked = (control_reg & 0xFF00) >> 8;
            auto idx = static_cast<int>(std::log2(idx_masked));
            return idx;
        }
        throw DebuggerError("Unsupported for current architecture");
    }

    static std::set<std::string> GetCallInstructions() {
        auto &ins = GetInstance();
        if (ins.current_machine == Machine::T86) {
            return {"CALL"};
        }
        throw DebuggerError("Unsupported for current architecture");
    }

    /// Returns set of instructions that can be used to exit
    /// a function.
    static std::set<std::string> GetReturnInstructions() {
        auto &ins = GetInstance();
        if (ins.current_machine == Machine::T86) {
            return {"RET"};
        }
        throw DebuggerError("Unsupported for current architecture");
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
