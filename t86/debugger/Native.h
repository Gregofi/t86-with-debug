#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <optional>

#include <fmt/core.h>

#include "Arch.h"
#include "Process.h"
#include "T86Process.h"
#include "Breakpoint.h"
#include "DebuggerError.h"
#include "common/TCP.h"

class Native {
public:
    Native(std::unique_ptr<Process> process): process(std::move(process)) { }
    /// Constructs instance of Native with debugging support for 
    /// machine set in Arch singleton.
    Native(int debug_port) {
        if (Arch::GetMachine() == Arch::Machine::T86) {
            process = std::make_unique<T86Process>(
                std::make_unique<TCP::TCPClient>(debug_port));
        } else {
            throw std::runtime_error("Specified machine is not supported");
        }
    }

    /// Returns SW BP opcode for current architecture.
    std::string_view GetSoftwareBreakpointOpcode() {
        static const std::map<Arch::Machine, std::string_view> opcode_map = {
            {Arch::Machine::T86, "BKPT"},
        };
        return opcode_map.at(Arch::GetMachine());
    }

    /// Creates new breakpoint at given address and enables it.
    std::optional<std::string> SetBreakpoint(uint64_t address) {
        if (software_breakpoints.contains(address)) {
            return fmt::format("Breakpoint at {} is already set!", address);
        }
        auto bp = CreateSoftwareBreakpoint(address);
        software_breakpoints.emplace(address, bp);
        return std::nullopt;
    }

    /// Disables and removes breakpoint from address.
    /// Throws if BP doesn't exist.
    void UnsetBreakpoint(uint64_t address) {
        DisableSoftwareBreakpoint(address);
        software_breakpoints.erase(address);
    }

    /// Enables BP at given address. If BP is already enabled then noop,
    /// if BP doesn't exist then throws DebuggerError.
    void EnableSoftwareBreakpoint(uint64_t address) {
        auto it = software_breakpoints.find(address);
        if (it == software_breakpoints.end()) {
            throw DebuggerError(fmt::format("No breakpoint at address {}!", address));
        }
        auto &bp = it->second;

        if (!bp.enabled) {
            bp = CreateSoftwareBreakpoint(address);
        }
    }

    /// Creates new enabled software breakpoint at given address.
    SoftwareBreakpoint CreateSoftwareBreakpoint(uint64_t address) {
        auto opcode = GetSoftwareBreakpointOpcode();
        auto backup = process->ReadText(address, 1).at(0);

        std::vector<std::string> data = {std::string(opcode)};
        process->WriteText(address, data);
   
        auto new_opcode = process->ReadText(address, 1).at(0);
        if (new_opcode != opcode) {
            throw DebuggerError(fmt::format(
                "Failed to set breakpoint! Expected opcode '{}', got '{}'",
                opcode, new_opcode));
        }

        return SoftwareBreakpoint{backup, true};
    }

    /// Disables BP at given address. If BP is already disabled then noop,
    /// if BP doesn't exist then throws DebuggerError.
    void DisableSoftwareBreakpoint(uint64_t address) {
        auto it = software_breakpoints.find(address);
        if (it == software_breakpoints.end()) {
            throw DebuggerError(fmt::format("No breakpoint at address {}!", address));
        }
        auto& bp = it->second;

        if (bp.enabled) {
            std::vector<std::string> prev_data = {bp.data};
            process->WriteText(address, prev_data);
            bp.enabled = false;
        }
    }

    void PerformSingleStep() {
        if (!Arch::SupportHardwareLevelSingleStep()) {
            // Requires instruction emulator.
            NOT_IMPLEMENTED;
        } else {
            process->Singlestep();
        }
    }
    
    void MovePCBack();

    int64_t ReadMemory();
protected:
    std::unique_ptr<Process> process;
    std::map<uint64_t, SoftwareBreakpoint> software_breakpoints;
};
