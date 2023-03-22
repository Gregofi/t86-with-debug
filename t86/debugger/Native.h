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
#include "Watchpoint.h"
#include "DebuggerError.h"
#include "DebugEvent.h"
#include "common/TCP.h"
#include "common/logger.h"

class Native {
public:
    Native(std::unique_ptr<Process> process): process(std::move(process)) {
    }

    /// Initializes native in an empty state.
    /// This native instance does NOT represent a process.
    Native() = default;

    /// Tries to connect to a process at given port and returns
    /// new Native object that represents that process.
    static std::unique_ptr<Process> Initialize(int port);

    // Removes all breakpoints and replaces them with bkpts.
    void SetAllBreakpoints(std::map<uint64_t, SoftwareBreakpoint> bkpts);

    // Removes all watchpoints and replaces them with provided watchpoints.
    void SetAllWatchpoints(std::map<uint64_t, Watchpoint> watchpoints);

    /// Creates new breakpoint at given address and enables it.
    void SetBreakpoint(uint64_t address);
    /// Disables and removes breakpoint from address.
    /// Throws if BP doesn't exist.
    void UnsetBreakpoint(uint64_t address);

    /// Enables BP at given address. If BP is already enabled then noop,
    /// if BP doesn't exist then throws DebuggerError.
    void EnableSoftwareBreakpoint(uint64_t address);

    /// Disables BP at given address. If BP is already disabled then noop,
    /// if BP doesn't exist then throws DebuggerError.
    void DisableSoftwareBreakpoint(uint64_t address);

    std::vector<std::string> ReadText(uint64_t address, size_t amount);

    void WriteText(uint64_t address, std::vector<std::string> text);

    /// Does singlestep, also steps over breakpoints.
    DebugEvent PerformSingleStep();

    size_t TextSize();

    std::map<std::string, double> GetFloatRegisters();

    void SetFloatRegisters(const std::map<std::string, double>& fregs);

    void SetFloatRegister(const std::string& name, double value);

    double GetFloatRegister(const std::string& name);

    std::map<std::string, int64_t> GetRegisters();

    /// Returns value of single register, if you need
    /// multiple registers use the FetchRegisters function
    /// which will be faster.
    /// Throws DebuggerError if register does not exist.
    int64_t GetRegister(const std::string& name);

    void SetRegisters(const std::map<std::string, int64_t>& regs);

    /// Sets one register to given value, throws DebuggerError if the register
    /// name is invalid. If setting multiple registers use the SetRegisters,
    /// which will be faster.
    void SetRegister(const std::string& name, int64_t value);

    uint64_t GetIP();

    void SetMemory(uint64_t address, const std::vector<int64_t>& values);

    std::vector<int64_t> ReadMemory(uint64_t address, size_t amount);

    DebugEvent MapReasonToEvent(StopReason reason);

    DebugEvent WaitForDebugEvent();

    void ContinueExecution();

    void SetWatchpointWrite(uint64_t address);

    void RemoveWatchpoint(uint64_t address);

    /// Returns watchpoints or empty map if the instance
    /// does not represent a running process.
    const std::map<uint64_t, Watchpoint>& GetWatchpoints();

    /// Returns breakpoints or empty map if the instance
    /// does not represent a running process.
    const std::map<uint64_t, SoftwareBreakpoint>& GetBreakpoints();

    void Terminate();

    bool Active() const;

    /// Performs step over, skipping any call instruction.
    /// If skip_bp is set then all breakpoints will be
    /// jumped over (the default behavior).
    DebugEvent PerformStepOver(bool skip_bp = true);

    /// Performs step out, stepping until 'RET' is executed.
    DebugEvent PerformStepOut();

    /// Does singlestep, does not check for breakpoints.
    DebugEvent DoRawSingleStep();
protected:
    void SetDebugRegister(uint8_t idx, uint64_t value);

    std::optional<size_t> GetFreeDebugRegister() const;

    /// Returns SW BP opcode for current architecture.
    std::string_view GetSoftwareBreakpointOpcode() const;

    /// Creates new enabled software breakpoint at given address.
    SoftwareBreakpoint CreateSoftwareBreakpoint(uint64_t address);

    /// Removes breakpoint at current ip,
    /// singlesteps and sets the breakpoint
    /// back. Return DebugEvent which occured
    /// from executing instruction at breakpoint.
    DebugEvent StepOverBreakpoint(size_t ip);

    std::unique_ptr<Process> process;
    std::map<uint64_t, SoftwareBreakpoint> software_breakpoints;
    std::map<uint64_t, Watchpoint> watchpoints;
    std::optional<DebugEvent> cached_event;
};
