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

/// Manages the native part of the debugging. Is responsible for all
/// logic behind native level debugging. It should know about the
/// bare minimum about the architecture it is debugging. And even then
/// it should use the Architecture class for this.
///
/// The class offers multiple getters and setters for various process
/// properties. For example getting and setting registers. The methods
/// are often provided for setting just one or multiple values at once.
/// If you value performance, always try to use the methods that sets/reads
/// multiple values at once if you need to set more than one value,
/// since that issues fewer calls to the underlying debugging API.
///
/// Design note: Although many of the methods seems to be const, they
/// issue a call to the underlying process which is either a network
/// or a shared thread memory thing. TODO: Think about making those members
/// mutable.
class Native {
public:
    /// Creates a new native instance.
    Native(std::unique_ptr<Process> process): process(std::move(process)) {
    }

    /// Creates a new native instance that does not represent a running process.
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

    /// Disables and removes breakpoint from an address.
    /// Throws if BP doesn't exist.
    void UnsetBreakpoint(uint64_t address);

    /// Enables BP at given address. If BP is already enabled then noop,
    /// if BP doesn't exist then throws DebuggerError.
    void EnableSoftwareBreakpoint(uint64_t address);

    /// Disables BP at given address. If BP is already disabled then noop,
    /// if BP doesn't exist then throws DebuggerError.
    void DisableSoftwareBreakpoint(uint64_t address);

    /// Returns text at given address.
    std::vector<std::string> ReadText(uint64_t address, size_t amount);

    /// Writes the values into the text at given address.
    void WriteText(uint64_t address, std::vector<std::string> text);

    /// Does singlestep, also steps over breakpoints.
    DebugEvent PerformSingleStep();

    /// Returns the size of the text section.
    size_t TextSize();

    /// Returns float registers.
    std::map<std::string, double> GetFloatRegisters();

    /// Sets Float registers.
    void SetFloatRegisters(const std::map<std::string, double>& fregs);

    /// Sets one float register.
    void SetFloatRegister(const std::string& name, double value);

    /// Returns one float register.
    double GetFloatRegister(const std::string& name);

    /// Returns normal registers.
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

    /// Returns the current value of instruction pointer.
    uint64_t GetIP();

    /// Modify the debugee memory.
    void SetMemory(uint64_t address, const std::vector<int64_t>& values);

    /// Read the debugee memory.
    std::vector<int64_t> ReadMemory(uint64_t address, size_t amount);

    /// Wait for debug event to occur.
    /// Should only be called after ContinueExecution.
    DebugEvent WaitForDebugEvent();

    /// Continue the execution of the debugee.
    /// No other call besides the WaitForDebugEvent
    /// should be issued after.
    void ContinueExecution();

    /// Set a watchpoint that will cause a break
    /// on a memory write to the given address.
    void SetWatchpointWrite(uint64_t address);

    /// Removes the watchpoint at given address.
    /// Throws if the watchpoint already exists.
    void RemoveWatchpoint(uint64_t address);

    /// Returns watchpoints or empty map if the instance
    /// does not represent a running process.
    const std::map<uint64_t, Watchpoint>& GetWatchpoints();

    /// Returns breakpoints or empty map if the instance
    /// does not represent a running process.
    const std::map<uint64_t, SoftwareBreakpoint>& GetBreakpoints();

    /// Terminates the debugee, no other calls should
    /// be made after this.
    void Terminate();

    /// Returns true if this instance represents a running process.
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
    /// Maps the DebugEvent to a reason.
    DebugEvent MapReasonToEvent(StopReason reason);

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
