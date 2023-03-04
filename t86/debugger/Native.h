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
#include "DebugEvent.h"
#include "common/TCP.h"
#include "common/logger.h"

class Native {
public:
    Native(std::unique_ptr<Process> process): process(std::move(process)) {

    }

    /// Tries to connect to a process at given port and returns
    /// new Native object that represents that process.
    static std::unique_ptr<Process> Initialize(int port) {
        auto tcp = std::make_unique<TCP::TCPClient>(port);
        tcp->Initialize();
        if (Arch::GetMachine() == Arch::Machine::T86) {
            return std::make_unique<T86Process>(std::move(tcp));
        } else {
            throw std::runtime_error("Specified machine is not supported");
        }
    }

    /// Creates new breakpoint at given address and enables it.
    void SetBreakpoint(uint64_t address) {
        if (software_breakpoints.contains(address)) {
            throw DebuggerError(
                fmt::format("Breakpoint at {} is already set!", address));
        }
        auto bp = CreateSoftwareBreakpoint(address);
        software_breakpoints.emplace(address, bp);
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

    std::vector<std::string> ReadText(uint64_t address, size_t amount) {
        // TODO: Needs to replace if some breakpoints are set.
        auto text_size = TextSize();
        if (address + amount > text_size) {
            throw DebuggerError(
                fmt::format("Reading text at range {}-{}, but text size is {}",
                            address, address + amount, text_size));
        }
        // If breakpoints we're set it will contain BKPT
        // instead of the current instruction, we remedy
        // that here.
        auto text = process->ReadText(address, amount);
        for (size_t i = 0; i < text.size(); ++i) {
            auto it = software_breakpoints.find(i + address);
            if (it != software_breakpoints.end()) {
                text.at(i) = it->second.data;
            }
        }
        return text;
    }

    void WriteText(uint64_t address, std::vector<std::string> text) {
        auto text_size = TextSize();
        if (address + text.size() > text_size) {
            throw DebuggerError(
                fmt::format("Writing text at range {}-{}, but text size is {}",
                            address, address + text.size(), text_size));
        }
        // Some of the code we want to rewrite might
        // be occupied by a breakpoint, so instead
        // we write it into its saved data and
        // persist the breakpoint in the debuggee.
        for (size_t i = 0; i < text.size(); ++i) {
            uint64_t curr_addr = i + address;
            auto bp = software_breakpoints.find(curr_addr);
            if (bp != software_breakpoints.end()) {
                bp->second.data = text[i];
                text[i] = GetSoftwareBreakpointOpcode();
            }
        }
        process->WriteText(address, text);
    }

    /// Does singlestep, also steps over breakpoints.
    DebugEvent PerformSingleStep() {
        if (!Arch::SupportHardwareLevelSingleStep()) {
            // Requires instruction emulator.
            throw DebuggerError(
                "Singlestep is not supported for current architecture");
        } else {
            auto ip = GetIP();
            auto bp = software_breakpoints.find(ip);
            if (bp != software_breakpoints.end() && bp->second.enabled) {
                return StepOverBreakpoint(ip);
            } else {
                return DoSingleStep();
            }
        }
    }

    size_t TextSize() {
        return process->TextSize();
    }

    std::map<std::string, int64_t> GetRegisters() {
        return process->FetchRegisters();
    }

    /// Returns value of single register, if you need
    /// multiple registers use the FetchRegisters function
    /// which will be faster.
    /// Throws DebuggerError if register does not exist.
    int64_t GetRegister(const std::string& name) {
        auto regs = process->FetchRegisters();
        auto reg = regs.find(name);
        if (reg == regs.end()) {
            throw DebuggerError(fmt::format("No register '{}' in target", name));
        }
        return reg->second;
    }

    void SetRegisters(const std::map<std::string, int64_t>& regs) {
        process->SetRegisters(regs);
    }

    /// Sets one register to given value, throws DebuggerError if the register
    /// name is invalid. If setting multiple registers use the SetRegisters,
    /// which will be faster.
    void SetRegister(const std::string& name, int64_t value) {
        auto regs = GetRegisters();
        if (regs.count(name) == 0) {
            // TODO: Make the error message more heplful (list the name of available
            // registers).
            throw DebuggerError(fmt::format("Unknown '{}' register name!", name)); 
        }
        regs.at(name) = value;
        SetRegisters(regs);
    }

    uint64_t GetIP() {
        // TODO: Not architecture independent (take IP name from Arch singleton)
        return GetRegister("IP"); 
    }

    DebugEvent WaitForDebugEvent() {
        DebugEvent reason;
        // If, for some reason, we got the event in some other
        // inner function (ie. ContinueExecution), return it now and empty it.
        if (cached_event) {
            reason = *cached_event;
            cached_event.reset();
        } else {
            process->Wait();
            reason = process->GetReason();
        }

        if (reason == DebugEvent::SoftwareBreakpointHit) {
            auto regs = GetRegisters();
            regs.at("IP") -= 1;
            SetRegisters(regs);
        }
        return reason;
    }

    void ContinueExecution() {
        auto ip = GetIP();
        auto bp = software_breakpoints.find(ip);
        // If breakpoint is not set
        if (bp == software_breakpoints.end() || !bp->second.enabled) {
            process->ResumeExecution();
        } else {
            auto event = StepOverBreakpoint(ip);
            // If some other thing happened other than singlestep
            // that requires pause cache the event here and return
            // it in WaitForDebugEvent.
            if (event != DebugEvent::Singlestep) {
                cached_event.emplace(event);
                return;
            }
            ContinueExecution();
        }
    }

    void Terminate() {
        process->Terminate();
    }
protected:
    /// Returns SW BP opcode for current architecture.
    std::string_view GetSoftwareBreakpointOpcode() {
        static const std::map<Arch::Machine, std::string_view> opcode_map = {
            {Arch::Machine::T86, "BKPT"},
        };
        return opcode_map.at(Arch::GetMachine());
    }

    /// Does singlestep, does not check for breakpoints.
    DebugEvent DoSingleStep() {
        process->Singlestep();
        return WaitForDebugEvent();
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

    /// Removes breakpoint at current ip,
    /// singlesteps and sets the breakpoint
    /// back. Return DebugEvent which occured
    /// from executing instruction at breakpoint.
    DebugEvent StepOverBreakpoint(size_t ip) {
        DisableSoftwareBreakpoint(ip);
        // Even though PerformSingleStep calls this function
        // it does not matter because we turn off the breakpoint
        // on the line above.
        auto event = PerformSingleStep();
        EnableSoftwareBreakpoint(ip); 
        return event;
    }

    std::unique_ptr<Process> process;
    std::map<uint64_t, SoftwareBreakpoint> software_breakpoints;
    std::optional<DebugEvent> cached_event;
};
