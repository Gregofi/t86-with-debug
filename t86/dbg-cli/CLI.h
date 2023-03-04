#pragma once

#include <iostream>
#include <string_view>
#include <cstdlib>
#include <cassert>

#include "common/TCP.h"
#include "common/helpers.h"
#include "common/logger.h"
#include "utility/linenoise.h"
#include "debugger/Native.h"

class CLI {
    static constexpr const char* BP_USAGE = 
R"(breakpoint <subcommads> [parameter [parameter...]]
If you want to set breakpoint at a specific address, the commands are 
equivalent but with the -addr postfix, for example to set a breakpoint at
address 1 you would use `breakpoint set-addr 1`.

- set <line> = Creates breakpoint at <line> and enables it.
- unset <line> = Removes breakpoint at <line>.
- enable <line> = Enables breakpoint at <line>.
- disable <line> = Disables breakpoint at <line>.
- list = Lists existing breakpoints.
- help = Print this.
)";
    static constexpr const char* STEPI_USAGE =
R"(stepi <subcommands> [parameter [parameter...]]
Used for instruction level single stepping

Without any subcommands (just stepi) = Performs instruction level single step.
- help = Print this.
)";
    static constexpr const char* DISASSEMBLE_USAGE =
R"(disassemble <subcommands> [parameter [parameter...]]
Used for disassembling the underlying T86 code.

Without any subcommands disassembles the current instruction and two below
and above it.
- range <b> <e> = Disassembles instructions from <b> to <e> inclusive.
- from <b> [<n>] = Dissasembles <n> instructions starting at <b>.
                   If <n> is not specified then disassembles the rest
                   of the executable starting from <b>.
)";
    static constexpr const char* USAGE =
R"(<command> <subcommand> [parameter [parameter...]]
Example: `breakpoint set-addr 1` sets breakpoint at address 1.

For help, use the `<command> help` syntax, for example
`breakpoint help`, or `disassemble help`.

Every command has its subcommands, except continue,
which should be used alone.

commands:
- continue = Continues execution, has no subcommands.
- stepi = Assembly level stepping.
- disassemble = Disassemble the underlying T86 code.
- breakpoint = Add and remove breakpoints.
- register = Read and write to registers
)";
    static constexpr const char* REGISTER_USAGE =
R"(register <subcommands> [parameter [parameter...]]
Used for reading and writing to debuggee registers. If
used without subcommand it'll dump all registers.

commands:
- set <reg> <val> - Sets the value of <reg> to <val>.
- get <reg> - Returns the value of <reg>.
)";

public:
    CLI(Native process): process(std::move(process)) {

    }

    /// Initializes connection to the process
    void OpenConnection(int port) {
        process.Initialize(port);
    }

    /// Dumps text around current IP
    /// @param range - How many instructions to dump above
    ///                and below current IP.
    void PrettyPrintText(uint64_t address, int range = 2) {
        size_t text_size = process.TextSize();
        if (address >= text_size) {
            return;
        }
        size_t begin = range > address ? 0 : address - range;
        size_t end = std::min(text_size, address + range + 1);
        auto inst = process.ReadText(begin, end - begin);
        for (size_t i = 0; i < inst.size(); ++i) {
            uint64_t curr_addr = i + begin;
            if (curr_addr == address) {
                fmt::print(fg(fmt::color::dark_blue), "-> {}\t{}\n", curr_addr, inst[i]);
            } else {
                fmt::print("   {}\t{}\n", curr_addr, inst[i]);
            }
        }
    }

    void PrintText(uint64_t address, const std::vector<std::string>& ins) {
        for (size_t i = 0; i < ins.size(); ++i) {
            uint64_t curr_addr = i + address;
            fmt::print("{}\t{}\n", curr_addr, ins[i]);
        }
    }

    uint64_t ParseAddress(std::string_view address) {
        auto loc = utils::svtoi64(address);
        if (!loc || loc < 0) {
            throw DebuggerError(fmt::format(
                "The disable-addr expected address, got '{}' instead",
                address));
        }
        return *loc;
    }

    std::string_view DebugEventToString(DebugEvent e) {
        switch (e) {
        case DebugEvent::SoftwareBreakpointHit: return "Software breakpoint hit";
        case DebugEvent::HardwareBreakpointHit: return "Hardware breakpoint hit";
        case DebugEvent::Singlestep: return "Singlestep done";
        case DebugEvent::ExecutionBegin: return "Execution started";
        case DebugEvent::ExecutionEnd: return "The program finished executing";
        }
        UNREACHABLE;
    }

    void HandleBreakpoint(std::string_view command) {
        // Breakpoint on raw address
        auto subcommands = utils::split_v(command);
        if (check_command(subcommands, "set-address", 2)) {
            auto address = ParseAddress(subcommands.at(1));
            auto line = process.ReadText(address, 1);
            process.SetBreakpoint(address);
            fmt::print("Breakpoint set on line {}: '{}'\n", address, line[0]);
        // Breakpoint on line
        } else if (check_command(subcommands, "set", 1)) {
            throw DebuggerError("Source breakpoints are not yet supported");
        } else if (check_command(subcommands, "disable-addr", 2)) {
            auto address = ParseAddress(subcommands.at(1));
            process.DisableSoftwareBreakpoint(address);
        } else if (check_command(subcommands, "enable-addr", 2)) {
            auto address = ParseAddress(subcommands.at(1));
            process.EnableSoftwareBreakpoint(address);
        } else if (check_command(subcommands, "remove-addr", 2)) {
            auto address = ParseAddress(subcommands.at(1));
            process.UnsetBreakpoint(address);
        } else if (check_command(subcommands, "help", 1)) {
            fmt::print("{}", BP_USAGE); 
        } else {
            fmt::print("{}", BP_USAGE); 
        }
    }

    void HandleStepi(std::string_view command) {
        if (command == "") {
            auto e = process.PerformSingleStep();
            if (e != DebugEvent::Singlestep) {
                fmt::print("Process stopped, reason: {}\n",
                           DebugEventToString(e));
            }
            auto ip = process.GetIP();
            PrettyPrintText(ip);
        } else if (utils::is_prefix_of(command, "help")) {
            fmt::print("{}", STEPI_USAGE);
        } else {
            fmt::print("{}", STEPI_USAGE);
        }
    }

    void HandleDisassemble(std::string_view command) {
        auto subcommands = utils::split_v(command);
        if (subcommands.size() == 0) {
            auto ip = process.GetIP();
            PrettyPrintText(ip);
        } else if (check_command(subcommands, "range", 3)) {
            auto begin = ParseAddress(subcommands.at(1));
            auto end = ParseAddress(subcommands.at(2));
            auto text = process.ReadText(begin, end - begin);
            PrintText(begin, text);
        } else if (check_command(subcommands, "from", 3)) {
            auto begin = ParseAddress(subcommands.at(1));
            // If to is not specified then disassemble the rest
            // of the file.
            auto amount = subcommands.size() > 2
                ? ParseAddress(subcommands.at(2))
                : process.TextSize() - begin;
            auto text = process.ReadText(begin, amount);
            PrintText(begin, text);
        } else if (check_command(subcommands, "help", 1)) {
            fmt::print("{}", DISASSEMBLE_USAGE);
        } else {
            fmt::print("{}", DISASSEMBLE_USAGE);
        }
    }

    void HandleRegister(std::string_view command) {
        auto subcommands = utils::split_v(command);
        if (subcommands.size() == 0) {
            auto regs = process.GetRegisters();
            for (const auto& [name, value]: regs) {
                fmt::print("{}:{}\n", name, value);
            }
        } else if (check_command(subcommands, "set", 3)) {
            auto reg = subcommands.at(1);
            auto value = utils::svtoi64(subcommands.at(2));
            if (!value) {
                throw DebuggerError(
                    fmt::format("Expected register value, instead got '{}'",
                                subcommands.at(2)));
            }
            process.SetRegister(std::string(reg), *value);
        } else if (check_command(subcommands, "get", 2)) {
            auto reg = subcommands.at(1);
            auto value = process.GetRegister(std::string(reg));
            fmt::print("{}\n", value);
        } else {
            fmt::print("{}", REGISTER_USAGE);
        }
    }

    void HandleContinue(std::string_view command) {
        if (!is_running) {
            throw DebuggerError("No process is running");
        }
        process.ContinueExecution();
        auto e = process.WaitForDebugEvent();
        if (e == DebugEvent::ExecutionEnd) {
            is_running = false;
        }
        fmt::print("Process stopped, reason: {}\n", DebugEventToString(e));
        auto ip = process.GetIP();
        PrettyPrintText(ip);
    }

    void HandleCommand(std::string_view command) {
        auto main_command = command.substr(0, command.find(' ') - 1);
        command = command.substr(main_command.size() +
                                 (command.size() != main_command.size()));
        if (utils::is_prefix_of(main_command, "breakpoint")) {
            HandleBreakpoint(command); 
        } else if (utils::is_prefix_of(main_command, "stepi")) {
            HandleStepi(command);
        } else if (utils::is_prefix_of(main_command, "disassemble")) {
            HandleDisassemble(command);
        } else if (utils::is_prefix_of(main_command, "continue")) {
            HandleContinue(command);
        } else if (utils::is_prefix_of(main_command, "register")) {
            HandleRegister(command);
        } else {
            fmt::print("{}", USAGE);
        }
    }

    int Run(bool attached = true) {
        linenoiseHistorySetMaxLen(100);
        if (attached) {
            auto reason = process.WaitForDebugEvent();
            assert(reason == DebugEvent::ExecutionBegin);
        }
        char* line_raw;
        while((line_raw = linenoise("> ")) != NULL) {
            std::string line = utils::squash_strip_whitespace(line_raw);
            if (line == "") {
                continue;
            }
            linenoiseHistoryAdd(line_raw);
            try {
                HandleCommand(line);
            } catch (const DebuggerError& err) {
                fmt::print(stderr, "Error: {}\n", err.what());
            }
            free(line_raw);
        }
        process.Terminate();
        return 0;
    }
private:
    /// Checks if subcommands is atleast of subcommand_size and then
    /// if the first subcommand is prefix of 'of'.
    bool check_command(const std::vector<std::string_view>& subcommands,
                       std::string_view of,
                       size_t subcommand_size) {
        if (subcommands.size() < subcommand_size) {
            return false;
        }
        return utils::is_prefix_of(subcommands[0], of);
    }

  Native process;
  bool is_running{true};

  static const int DEFAULT_DBG_PORT = 9110;
};
