#pragma once

#include <iostream>
#include <fstream>
#include <string_view>
#include <cstdlib>
#include <cassert>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/core.h>

#include "common/TCP.h"
#include "common/helpers.h"
#include "common/logger.h"
#include "debugger/Source/Parser.h"
#include "debugger/Source/Source.h"
#include "t86/os.h"
#include "threads_messenger.h"
#include "utility/linenoise.h"
#include "debugger/Native.h"
#include "debugger/Source/ExpressionInterpreter.h"

/// Checks if subcommands is atleast of subcommand_size and then
/// if the first subcommand is prefix of 'of'.
static bool check_command(const std::vector<std::string_view>& subcommands,
                          std::string_view of,
                          size_t subcommand_size) {
    if (subcommands.size() < subcommand_size) {
        return false;
    }
    return utils::is_prefix_of(subcommands[0], of);
}

template<typename ...Args>
void Error(fmt::format_string<Args...> format_str, Args&& ...args) {
    throw DebuggerError(fmt::format(format_str, std::forward<Args>(args)...));
}

class Cli {
    static constexpr const char* USAGE =
R"(<command> <subcommand> [parameter [parameter...]]
Example: `breakpoint iset 1` sets breakpoint at address 1.

For help, use the `help <command>` syntax, for example
`help breakpoint`, or `help disassemble`.

Every command has its subcommands unless explicitly specified.
Some of the commands work with or without subcommands.

commands:
- help = Display help message about subcommands.
- run = Run the program.
- attach <port> = Attach to an already running VM.
- continue = Continue execution.
- breakpoint = Stop the program at various points.
- istep = Execute one instruction.
- step = Execute one source line.
- inext = Step over call instructions.
- next = Step over function calls.
- finish = Leave current function.
- disassemble = Disassemble the underlying native code.
- assemble = Rewrite the underlying native code.
- register = Read and write to registers.
- watchpoint = Watch for writes to values in memory.
- frame = Print information about current function and variables.
- expression = Evaluate the source language expression and print result.
- source = Print the source code that is being debugged.
)";
    static constexpr const char* RUN_USAGE =
R"(run [--arg=val [--arg=val ...]]
Run the program, stopping before the first instruction is executed.

options:
--reg-count=<cnt> - Number of normal registers.
--float-reg-count=<cnt> - Number of float registers.
--data-size=<cnt> - Size of the RAM memory.
)";
    static constexpr const char* BP_USAGE = 
R"(breakpoint <subcommads> [parameter [parameter...]]
Make program stop at certain point.

To work with breakpoints at instruction level, just prefix
the commands with the 'i' (as instruction), ie. `bp iset 1` sets 
breakpoint on instruction at address 1.

- set <line> = Creates breakpoint at <line> and enables it.
- unset <line> = Removes breakpoint at <line>.
- enable <line> = Enables breakpoint at <line>.
- disable <line> = Disables breakpoint at <line>.
- list = Lists existing breakpoints.
)";
    static constexpr const char* ISTEP_USAGE =
R"(istep
Execute one instruction and then stop.
)";
    static constexpr const char* INEXT_USAGE =
R"(inext
Execute one instruction and then stop.
Treat calls as a single instruction (effectively stepping over them).
)";

    static constexpr const char* STEP_USAGE =
R"(step
Continue execution until another source line is reached.
)";
    static constexpr const char* NEXT_USAGE =
R"(next <subcommands> [parameter [parameter...]]
Continue execution until another source line is reached.
Treat calls as a single source line (effectively stepping over them).
)";
    static constexpr const char* DISASSEMBLE_USAGE =
R"(disassemble <subcommands> [parameter [parameter...]]
Used for disassembling the underlying assembly.

Without any subcommands disassembles the current instruction and two below
and above it.
- range <b> <e> = Disassembles instructions from <b> to <e> inclusive.
- from <b> [<n>] = Dissasembles <n> instructions starting at <b>.
                   If <n> is not specified then disassembles the rest
                   of the executable starting from <b>.
- function <name> = Disassemble function body, needs debugging information
                    about the function beginning and ending address.
)";
    static constexpr const char* REGISTER_USAGE =
R"(register <subcommands> [parameter [parameter...]]
Used for reading and writing to debuggee registers. If
used without subcommand it'll dump all registers.

commands:
- set <reg> <val> - Sets the value of <reg> to <val>.
- get <reg> - Returns the value of <reg>.
- fset <freg> <double> - Sets the value of float register <freg> to <double>.
- fget <freg> - Returns the value of float register <reg>.
)";
    static constexpr const char* ASSEMBLE_USAGE =
R"(assemble <subcommands> [parameter [parameter...]]
Rewrite the underlying assembly.
Warning: Does not support writing outside the text size (you can't add new instructions).

commands:
- interactive <from> - Start rewriting from address <from>. Enter instruction and newline.
                       Empty line means end of rewriting.
)";
    static constexpr const char* MEMORY_USAGE =
R"(memory <subcommands> [parameter [parameter...]]
Write and read into the .data section (the RAM memory) of the program.

commands:
- get <addr> <amount> - Reads an <amount> of memory cells beginning from <addr>
- gets <addr> <amount> - Reads an <amount> of memory cells beginning from <addr>
                               interpret the result as a c-string. If the string
                               is missing newline at the end it is appended and
                               together with a '\%' sequence.
- set <addr> <value> [<value>...] - Write values, beginning at address <addr>.
- sets <addr> <string> - Write string encoded in ASCII on address <addr>. The
                         terminating zero is appended automatically. The string
                         is in form "Hello, World!\n", it must begin and end with
                         quotation marks.
)";
    static constexpr const char* WATCHPOINT_USAGE =
R"(watchpoint <subcommands> [parameter [parameter...]]
Watchpoint will watch for memory writes and break if any happen.

commands:
- iset <addr> - Creates a new watchpoint that will cause a
                break if any write happens to <addr>.
- irem <addr> - Remove watchpoint on address <addr>.
- list - Lists all active watchpoints.
)";
    static constexpr const char* SOURCE_USAGE =
R"(source <subcommands> [parameter [parameter...]]
Print the debugged source code if enough debugging information
is available.

commands:
- from <line> [<amount>] - Print source code from given line, if
                           amount is not specified then it prints
                           the rest of the source.
- without subcommands - Print the current line and few lines around.
)";
    static constexpr const char* EXPRESSION_USAGE =
R"(expression <expression>
Evaluate an expression and print corresponding value.
If you have a variable 'a' and 'b' in scope, you can write
'a + b' to get their sum. If one of those is a pointer, you
can do '*a + b', or if struct: 'a.foo + b'. You can do almost
everything that you can in C, even the assignment operator is
supported. Do however note that function calls are unsupported.
)";
    static constexpr const char* FINISH_USAGE =
R"(finish
Execute until a return is executed.
)";
    static constexpr const char* FRAME_USAGE =
R"(frame
Display current function and active variables.
)";
    static constexpr const char* CONTINUE_USAGE =
R"(continue
Continue execution until a debug event happens.
)";

public:
    Cli(std::string fname): fname(std::move(fname)) {
    }

    Cli() = default;


    /// Parses string from given stringview, converts most
    /// of the escape characters ('\' + 'n' -> '\n') into
    /// one and checks if it begins and ends with '"'.
    std::string ParseString(std::string_view s) {
        if (s.size() < 2 || !s.starts_with('"') || !s.ends_with('"')) {
            Error("Expected string to begin and end with '\"'");
        }
        s.remove_prefix(1);
        std::string result;
        char prev = s.at(0);
        for (size_t i = 1; i < s.size(); ++i) {
            if (prev == '\\') {
                if (s[i] == 'n') {
                    result += '\n';
                } else if (s[i] == 't') {
                    result += '\t';
                } else if (s[i] == '0') {
                    result += '\0';
                } else {
                    Error("Unknown escape sequence '{}{}'", prev, s[i]);
                }
                prev = s[++i];
            } else {
                result += prev;
                prev = s[i];
            }
        }
        return result;
    }

    /// Prints current function and currently active variables (if available).
    /// if var_detailed is set to true then the values of the variables are
    /// printed as well.
    void PrintFunctionInfo(uint64_t address, bool var_detailed = false) {
        auto fun_name = source.GetFunctionNameByAddress(address);
        if (!fun_name) {
            return;
        }
        auto fun_loc = source.GetFunctionAddrByName(*fun_name);
        if (!fun_loc) {
            return;
        }
        auto [fun_begin, fun_end] = *fun_loc;
        auto vars = source.GetScopedVariables(address);
        std::string variables = "";
        if (!var_detailed) {
            auto it = vars.begin();
            if (it != vars.end()) {
                variables += "; active variables: " + *it++;
            }
            while (it != vars.end()) {
                variables += ", " + *it++;
            }
        }
        fmt::print("function {} at {}-{}{}\n", fmt::styled(*fun_name,
                    fmt::emphasis::bold | fmt::fg(fmt::color::dark_green)),
                    fun_begin, fun_end, variables);
        if (var_detailed) {
            for (auto&& varname: vars) {
                try {
                    auto [val, idx] = source.EvaluateExpression(process, std::string{varname}, false);
                    auto value_str = source.TypedValueToString(process, val);
                    auto type = source.GetVariableTypeInformation(process, varname);
                    if (!type) {
                        continue;
                    }
                    auto type_str = source.TypeToString(*type);
                    fmt::print("({}) {} = {}\n", type_str, varname, value_str);
                } catch (const DebuggerError& e) {
                    // If debug info is missing about some var we
                    // will silently ignore it.
                    log_info("frame: debug info missing: {}", e.what());
                }
            }
        }
    }

    /// Dumps text around current IP
    /// @param range - How many instructions to dump above
    ///                and below current IP.
    void PrettyPrintText(uint64_t address, int range = 2) {
        size_t text_size = process.TextSize();
        if (address >= text_size) {
            return;
        }
        size_t begin = static_cast<size_t>(range) > address ? 0 : address - range;
        size_t end = std::min(text_size, static_cast<size_t>(address) + range + 1);
        auto inst = process.ReadText(begin, end - begin);
        PrintFunctionInfo(address);
        PrintText(begin, inst);
    }

    /// Prints source code around given line, if no line exists then
    /// nothing is printed.
    void PrettyPrintCode(ssize_t line, int range = 2) {
        auto begin = std::max(line - range, 0l);
        auto end = line + range + 1;
        PrintFunctionInfo(process.GetIP());
        PrintCode(begin, end);
    }

    /// Returns red '@' if enabled or gray if disabled. If breakpoint is not set on i then
    /// returns a space (' ').
    std::string GetPrettyBp(uint64_t i, const std::map<uint64_t, SoftwareBreakpoint>& bps) {
        std::string res = " ";
        if (bps.contains(i) > 0) {
            if (bps.at(i).enabled) {
                res = fmt::format(fg(fmt::color::red), "@");
            } else {
                res = fmt::format(fg(fmt::color::gray), "@");
            }
        }
        return res;
    }

    std::string GetPrettyBpForLine(size_t line) {
        const auto& bkpts = process.GetBreakpoints();
        auto addr = source.LineToAddr(line);
        if (!addr) {
            return " ";
        }
        // One line may map to multiple addresses,
        // but this function always returns the last
        // line the address is mapped onto.
        // This is done to prevent displaying breakpoints
        // on multiple lines belonging to the same instruction.
        auto line_back = source.AddrToLine(*addr);
        if (line_back && line == *line_back) {
            return GetPrettyBp(*addr, bkpts);
        }
        return " ";
    }

    void PrintCode(size_t from, size_t to) {
        auto curr_loc = source.AddrToLine(process.GetIP());
        for (size_t i = from; i < to; ++i) {
            auto line = source.GetLine(i);
            if (!line) {
                continue;
            }
            std::string pretty_rest;
            if (curr_loc && i == *curr_loc) {
                pretty_rest = fmt::format(fg(fmt::color::dark_blue), "->{:>4}:{}\n", i + 1, *line);
            } else {
                pretty_rest = fmt::format("  {:>4}:{}\n", i + 1, *line);
            }
            fmt::print("{}{}", GetPrettyBpForLine(i), pretty_rest);
        }
    }

    /// Prints the provided text and markups current address and breakpoints.
    void PrintText(uint64_t address, const std::vector<std::string>& ins) {
        auto ip = process.GetIP();
        const auto& bkpts = process.GetBreakpoints();
        for (size_t i = 0; i < ins.size(); ++i) {
            uint64_t curr_addr = i + address;
            auto bp_s = GetPrettyBp(curr_addr, bkpts);
            std::string pretty_rest;
            if (curr_addr == ip) {
                pretty_rest = fmt::format(fg(fmt::color::dark_blue), "->{:>4}:  {}\n", curr_addr, ins[i]);
            } else {
                pretty_rest = fmt::format("  {:>4}:  {}\n",  curr_addr, ins[i]);
            }
            // The print itself has to be done separately, because the style
            // of breakpoints would cancel out the style of the instruction.
            fmt::print("{}{}", bp_s, pretty_rest);
        }
    }

    uint64_t ParseAddress(std::string_view address) {
        auto loc = utils::svtoi64(address);
        if (!loc || loc < 0) {
            Error("Expected unsigned number, got '{}' instead", address);
        }
        return *loc;
    }

    std::string DebugEventToString(const DebugEvent& e) {
        using namespace std::literals;
        return std::visit(utils::overloaded {
            [&](const BreakpointHit& r) {
                auto line = source.AddrToLine(r.address);
                if (line) {
                    return fmt::format("Software breakpoint hit at line {}", *line + 1);
                } else {
                    return fmt::format("Software breakpoint hit at address {}", r.address);
                }
            },
            [](const WatchpointTrigger& r) {
                return fmt::format("Watchpoint triggered at memory address {}", r.address);
            },
            [](const Singlestep& r) {
                return "Singlestep done"s;
            },
            [](const ExecutionBegin& r) {
                return "Execution started"s;
            },
            [](const ExecutionEnd& r) {
                return "The program finished execution"s;
            },
            [](const CpuError& r) {
                return "Inner CPU error occured."s;
            },
        }, e);
    }

    void ReportDebugEvent(const DebugEvent& e) {
        fmt::print("Process stopped, reason: {}\n",
                fmt::styled(DebugEventToString(e),
                    fmt::emphasis::bold | fg(fmt::color::dark_red)));
        if (std::holds_alternative<CpuError>(e)) {
            const char* message =
R"(The CPU is now in undefined state, you can try to
fetch information about it, but be aware that the information
may not be correct. Inspect the VM logs to see what kind of
error happened. Continuing execution will cause the CPU
to exit. We will provide an approximate address where the
exception happened, but it will probably not be correct because
we hadn't had a chance to unroll current speculations.
Most often, the correct address will be one below it.)";
            fmt::print(fg(fmt::color::red), "{}\n", message);
            fmt::print("The error probably happened on address '{}'\n", process.GetIP());
            is_running = false;
        }
    }

    /// Prints code or text at current location, depending on available info.
    void ReportBreak(const DebugEvent& e) {
        // The IP does not correspond to the HALT instruction,
        // it is one past it, rather not print anything.
        if (std::holds_alternative<ExecutionEnd>(e)) {
            return;
        }
        auto ip = process.GetIP();
        auto line = source.AddrToLine(ip);
        // If we stopped on a line then print that, otherwise print assembly.
        if (line) {
            PrettyPrintCode(*line);
        } else {
            PrettyPrintText(ip);
        }
    }

    void HandleBreakpoint(std::string_view command) {
        // TODO: Lot of repeated code
        if (!process.Active()) {
            Error("No active process.");
        }
        // Breakpoint on raw address
        auto subcommands = utils::split_v(command);
        if (check_command(subcommands, "iset", 2)) {
            auto address = ParseAddress(subcommands.at(1));
            auto line = process.ReadText(address, 1);
            process.SetBreakpoint(address);
            fmt::print("Breakpoint set on address {}: '{}'\n", address, line[0]);
        // Breakpoint on line
        } else if (check_command(subcommands, "set", 2)) {
            uint64_t addr = source.GetAddressFromString(subcommands.at(1), true);
            process.SetBreakpoint(addr);
            auto line = source.AddrToLine(addr);
            auto source_line = source.GetLine(*line);
            fmt::print("Breakpoint set on line {} (addr {})", *line + 1, addr);
            if (source_line) {
                fmt::print(": {}\n", *source_line);
            } else {
                fmt::print("\n");
            }
        } else if (check_command(subcommands, "remove", 2)) {
            uint64_t addr = source.GetAddressFromString(subcommands.at(1), true);
            process.UnsetBreakpoint(addr);
            auto line = source.AddrToLine(addr);
            assert(line);
            auto source_line = source.GetLine(*line);
            fmt::print("Breakpoint removed from line {} (addr {})", *line + 1, addr);
            if (source_line) {
                fmt::print(": {}\n", *source_line);
            } else {
                fmt::print("\n");
            }
        } else if (check_command(subcommands, "enable", 2)) {
            uint64_t addr = source.GetAddressFromString(subcommands.at(1), true);
            process.EnableSoftwareBreakpoint(addr);
            auto line = source.AddrToLine(addr);
            assert(line);
            auto source_line = source.GetLine(*line);
            fmt::print("Breakpoint enabled on line {} (addr {})", *line + 1, addr);
            if (source_line) {
                fmt::print(": {}\n", *source_line);
            } else {
                fmt::print("\n");
            }
        } else if (check_command(subcommands, "disable", 2)) {
            uint64_t addr = source.GetAddressFromString(subcommands.at(1), true);
            process.DisableSoftwareBreakpoint(addr);
            auto line = source.AddrToLine(addr);
            assert(line);
            auto source_line = source.GetLine(*line);
            fmt::print("Breakpoint disabled on line {} (addr {})", *line + 1, addr);
            if (source_line) {
                fmt::print(": {}\n", *source_line);
            } else {
                fmt::print("\n");
            }
        } else if (check_command(subcommands, "idisable", 2)) {
            auto address = ParseAddress(subcommands.at(1));
            process.DisableSoftwareBreakpoint(address);
            fmt::print("Breakpoint disabled at address {}\n", address);
        } else if (check_command(subcommands, "ienable", 2)) {
            auto address = ParseAddress(subcommands.at(1));
            process.EnableSoftwareBreakpoint(address);
            fmt::print("Breakpoint enabled at address {}\n", address);
        } else if (check_command(subcommands, "iremove", 2)) {
            auto address = ParseAddress(subcommands.at(1));
            process.UnsetBreakpoint(address);
            fmt::print("Breakpoint removed from address {}\n", address);
        } else if (check_command(subcommands, "list", 1)) {
            auto& bps = process.GetBreakpoints();
            for (auto&& [addr, b]: bps) {
                auto line = source.AddrToLine(addr);
                fmt::print("addr: {}{}; {}", addr,
                        line ? fmt::format(", line: {}", *line) : "",
                        b.enabled ? "enabled" : "disabled");
                auto fun = source.GetFunctionNameByAddress(addr);
                if (fun) {
                    fmt::print("; function: {}", *fun);
                }
                fmt::print("\n");
            }
        } else {
            fmt::print("{}", BP_USAGE); 
        }
    }

    void HandleWatchpoint(std::string_view command) {
        auto subcommands = utils::split_v(command);
        if (check_command(subcommands, "iset", 2)) {
            auto address = ParseAddress(subcommands.at(1));
            process.SetWatchpointWrite(address);
        } else if (check_command(subcommands, "irem", 2)) {
            auto address = ParseAddress(subcommands.at(1));
            process.RemoveWatchpoint(address);
        } else if (check_command(subcommands, "list", 1)) {
            const auto& watchpoints = process.GetWatchpoints();
            if (watchpoints.size() == 0) {
                fmt::print("No active watchpoints\n");
                return;
            }

            fmt::print("Active watchpoints:\n");
            for (auto&& w: watchpoints) {
                auto&& [address, wp] = w;
                fmt::print(" - address {}\n", address);
            }
        } else {
            fmt::print("{}", WATCHPOINT_USAGE);
        }
    }

    void SourceLevelStep(bool step_over = false) {
        DebugEvent e;
        if (step_over) {
            e = source.StepOver(process);
        } else {
            e = source.StepIn(process);
        }
        if (!std::holds_alternative<Singlestep>(e)) {
            ReportDebugEvent(e);
        }
        if (std::holds_alternative<ExecutionEnd>(e)) {
            is_running = false;
        } else {
            auto ip = process.GetIP();
            auto line = source.AddrToLine(ip);
            // The line should be valid because source level single step
            // was made but it is possible that no source info is available
            // and source step was invoked anyway.
            if (!line) {
                return;
            }
            PrettyPrintCode(*line);
        }
    }

    void HandleStep(std::string_view command) {
        if (!process.Active()) {
            Error("No active process.");
        }
        if (!is_running) {
            Error("Process finished executing, it's not possible to continue.");
        }
        if (command == "") {
            SourceLevelStep();
        } else {
            fmt::print("{}", STEP_USAGE);
        }
    }

    void HandleNext(std::string_view command) {
        if (!process.Active()) {
            Error("No active process.");
        }
        if (!is_running) {
            Error("Process finished executing, it's not possible to continue.");
        }
        if (command == "") {
            SourceLevelStep(true);
        } else {
            fmt::print("{}", NEXT_USAGE);
        }
    }

    void HandleFinish(std::string_view command) {
        if (!process.Active()) {
            Error("No active process.");
        }
        if (!is_running) {
            Error("Process finished executing, it's not possible to continue.");
        }
        if (command == "") {
            auto e = process.PerformStepOut();
            if (!std::holds_alternative<Singlestep>(e)) {
                ReportDebugEvent(e);
            }
            if (std::holds_alternative<ExecutionEnd>(e)) {
                is_running = false;
            } else {
                auto ip = process.GetIP();
                auto line = source.AddrToLine(ip);
                if (!line) {
                    PrettyPrintText(ip);
                } else {
                    PrettyPrintCode(*line);
                }
            }
        } else {
            fmt::print("{}", FINISH_USAGE);
        }
    } 

    void NativeLevelStep(bool step_over = false) {
        DebugEvent e;
        if (step_over) {
            e = process.PerformStepOver();
        } else {
            e = process.PerformSingleStep();
        }
        if (!std::holds_alternative<Singlestep>(e)) {
            ReportDebugEvent(e);
        }
        if (std::holds_alternative<ExecutionEnd>(e)) {
            is_running = false;
        } else {
            // Instruction level stepping should always
            // display the instructions even if they
            // have source line mapping.
            auto ip = process.GetIP();
            PrettyPrintText(ip);
        }
    }

    void HandleStepi(std::string_view command) {
        if (!process.Active()) {
            Error("No active process.");
        }
        if (!is_running) {
            Error("Process finished executing, it's not possible to continue.");
        }
        if (command == "") {
            NativeLevelStep();
        } else {
            fmt::print("{}", ISTEP_USAGE);
        }
    }

    void HandleINext(std::string_view command) {
        // TODO: Think about if inext and istep couldn't be unified.
        if (!process.Active()) {
            Error("No active process.");
        }
        if (!is_running) {
            Error("Process finished executing, it's not possible to continue.");
        }
        if (command == "") {
            NativeLevelStep(true);
        } else {
            fmt::print("{}", INEXT_USAGE);
        }
    }

    void HandleDisassemble(std::string_view command) {
        if (!process.Active()) {
            Error("No active process.");
        }
        auto subcommands = utils::split_v(command);
        if (subcommands.size() == 0) {
            auto ip = process.GetIP();
            PrettyPrintText(ip);
        } else if (check_command(subcommands, "range", 3)) {
            auto begin = ParseAddress(subcommands.at(1));
            auto end = ParseAddress(subcommands.at(2));
            auto text = process.ReadText(begin, end - begin);
            PrintText(begin, text);
        } else if (check_command(subcommands, "from", 2)) {
            auto begin = ParseAddress(subcommands.at(1));
            // If to is not specified then disassemble the rest
            // of the file.
            size_t text_size = process.TextSize();
            if (begin >= text_size) {
                Error("Size of text is '{}'", text_size);
            }
            auto amount = subcommands.size() > 2
                ? ParseAddress(subcommands.at(2))
                : text_size - begin;
            auto text = process.ReadText(begin, amount);
            PrintText(begin, text);
        } else if (check_command(subcommands, "function", 2)) {
            auto id = subcommands.at(1);
            auto fun_addr = source.GetFunctionAddrByName(id);
            if (!fun_addr) {
                Error("No debug info about function '{}'", id);
            }
            auto text = process.ReadText(fun_addr->first,
                    fun_addr->second - fun_addr->first);
            PrintText(fun_addr->first, text);
        } else {
            fmt::print("{}", DISASSEMBLE_USAGE);
        }
    }

    void HandleSource(std::string_view command) {
        if (!process.Active()) {
            Error("No active process.");
        }
        auto subcommands = utils::split_v(command);
        if (subcommands.size() == 0) {
            auto ip = process.GetIP();
            std::optional<ssize_t> line = source.AddrToLine(ip);
            if (!line) {
                Error("Cannot map current address ({}) to line", ip);
            }
            auto begin = std::max(*line - 2, 0l);
            auto end = *line + 2 + 1;
            PrintCode(begin, end);
        } else if (check_command(subcommands, "from", 2)) {
            auto from_line = ParseAddress(subcommands.at(1)) - 1;
            auto amount = subcommands.size() >= 3
                            ? ParseAddress(subcommands.at(2))
                            : source.GetLines().size() - from_line;
            PrintCode(from_line, from_line + amount);
        } else {
            fmt::print("{}", SOURCE_USAGE);
        }
    }

    void InteractiveAssemble(size_t address) {
        char* line_raw;
        // TODO: Save and rollback linenoise history
        std::vector<std::string> result;
        for (size_t i = 0; ;++i) {
            line_raw = linenoise(fmt::format("{}: > ", address + i).c_str());
            if (line_raw == NULL) {
                Error("Unexpected end of input in interactive assembling, "
                      "use blank line to indicate the end.");
            }
            std::string line{line_raw};
            if (line == "") {
                free(line_raw);
                break;
            }
            result.push_back(line);
            linenoiseHistoryAdd(line_raw);
            free(line_raw);
        }
        process.WriteText(address, result);
    }

    void HandleAssemble(std::string_view command) {
        if (!process.Active()) {
            Error("No active process.");
        }
        auto subcommands = utils::split_v(command);
        if (check_command(subcommands, "interactive", 2)) {
            auto addr = utils::svtonum<size_t>(subcommands.at(1));
            if (!addr) {
                Error("Expected address, got '{}'", subcommands.at(2));
            }
            InteractiveAssemble(*addr);
        } else {
            fmt::print("{}", ASSEMBLE_USAGE);
        }
    }

    void HandleRegister(std::string_view command) {
        if (!process.Active()) {
            Error("No active process.");
        }
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
                Error("Expected register value, instead got '{}'", subcommands.at(2));
            }
            process.SetRegister(std::string(reg), *value);
        } else if (check_command(subcommands, "get", 2)) {
            auto reg = subcommands.at(1);
            auto value = process.GetRegister(std::string(reg));
            fmt::print("{}\n", value);
        } else if (check_command(subcommands, "fget", 2)) {
            auto reg = subcommands.at(1);
            auto value = process.GetFloatRegister(std::string(reg));
            fmt::print("{}\n", value);
        } else if (check_command(subcommands, "fset", 3)) {
            auto reg = subcommands.at(1);
            auto value = utils::svtonum<double>(subcommands.at(2));
            if (!value) {
                Error("Expected register value, instead got '{}'", subcommands.at(2));
            }
            process.SetFloatRegister(std::string(reg), *value);
        } else {
            fmt::print("{}", REGISTER_USAGE);
        }
    }

    void HandleMemory(std::string_view command) {
        if (!process.Active()) {
            Error("No process is running");
        }
        auto subcommands = utils::split_v(command);
        if (check_command(subcommands, "set", 3)) {
            auto cell = utils::svtonum<size_t>(subcommands.at(1));
            std::vector<int64_t> values;
            std::transform(std::next(subcommands.begin(), 2), subcommands.end(), 
                    std::back_inserter(values), [&](auto&& s) { 
                auto v = utils::svtonum<int64_t>(s);
                if (!v) {
                    Error("Expected number, got '{}'", s);
                }
                return *v;
            });
            process.SetMemory(*cell, values);
        } else if (check_command(subcommands, "setstr", 3)) {
            auto cell = utils::svtonum<size_t>(subcommands.at(1));
            if (!cell) {
                Error("Expected memory cell and string");
            }
            auto str = utils::join(std::next(subcommands.begin(), 2), subcommands.end());
            auto unescaped_str = ParseString(str);
            log_info("setstr: string = '{}'", unescaped_str);
            std::vector<int64_t> data;
            std::transform(unescaped_str.begin(), unescaped_str.end(),
                    std::back_inserter(data), [](auto&& c){ return c; });
            // The terminating zero
            data.push_back(0);
            process.SetMemory(*cell, data);
        } else if (check_command(subcommands, "get", 3)
                || check_command(subcommands, "getstr", 3)) {
            auto begin = utils::svtonum<size_t>(subcommands.at(1));
            auto amount = utils::svtonum<size_t>(subcommands.at(2));
            if (!begin || !amount) {
                Error("Expected range of memory cells, instead got '{}', '{}'",
                        subcommands.at(1), subcommands.at(2));
            }
            auto vals = process.ReadMemory(*begin, *amount);
            if (subcommands.at(0).starts_with("gets")) {
                for (size_t i = 0; i < vals.size(); ++i) {
                    fmt::print("{}", static_cast<char>(vals[i]));
                    if (i + 1 == vals.size() && vals[i] != '\n') {
                        fmt::print("\\%\n");
                    }
                }
            } else {
                for (auto&& val: vals) {
                    fmt::print("{}:{}\n", (*begin)++, val);
                }
            }
        } else {
            fmt::print("{}", MEMORY_USAGE);
        }
    }

    void HandleFrame(std::string_view command) {
        if (!process.Active()) {
            Error("No active process.");
        }
        if (!is_running) {
            Error("Process finished executing, no frame information available.");
        }
        PrintFunctionInfo(process.GetIP(), true);
    }

    void HandleContinue(std::string_view command) {
        if (!process.Active()) {
            Error("No active process.");
        }
        if (!is_running) {
            Error("Process finished executing, it's not possible to continue.");
        }
        process.ContinueExecution();
        auto e = process.WaitForDebugEvent();
        if (std::holds_alternative<ExecutionEnd>(e)) {
            is_running = false;
        }
        ReportDebugEvent(e);
        ReportBreak(e);
    }

    void HandleExpression(std::string_view command) {
        if (!process.Active()) {
            Error("No active process.");
        }
        if (command == "") {
            fmt::print("{}", EXPRESSION_USAGE);
            return;
        }
        auto [val, idx] = source.EvaluateExpression(process, std::string{command});
        fmt::print("({}) ${} = {}\n", source.TypedValueTypeToString(val),
                                      idx, source.TypedValueToString(process, val));
    }

    void HandleHelp(std::string_view command) {
        if (command == "") {
            fmt::print("{}", USAGE);
        } else if (utils::is_prefix_of(command, "breakpoint")) {
            fmt::print("{}", BP_USAGE);
        } else if (utils::is_prefix_of(command, "istep")) {
            fmt::print("{}", ISTEP_USAGE);
        } else if (utils::is_prefix_of(command, "inext")) {
            fmt::print("{}", INEXT_USAGE);
        } else if (utils::is_prefix_of(command, "run")) {
            fmt::print("{}", RUN_USAGE);
        } else if (utils::is_prefix_of(command, "disassemble")) {
            fmt::print("{}", DISASSEMBLE_USAGE);
        } else if (utils::is_prefix_of(command, "assemble")) {
            fmt::print("{}", ASSEMBLE_USAGE);
        } else if (utils::is_prefix_of(command, "continue")) {
            fmt::print("{}", CONTINUE_USAGE);
        } else if (utils::is_prefix_of(command, "register")) {
            fmt::print("{}", REGISTER_USAGE);
        } else if (utils::is_prefix_of(command, "memory")) {
            fmt::print("{}", MEMORY_USAGE);
        } else if (utils::is_prefix_of(command, "watchpoint")) {
            fmt::print("{}", WATCHPOINT_USAGE);
        } else if (utils::is_prefix_of(command, "next")) {
            fmt::print("{}", NEXT_USAGE);
        } else if (utils::is_prefix_of(command, "step")) {
            fmt::print("{}", STEP_USAGE);
        } else if (utils::is_prefix_of(command, "finish")) {
            fmt::print("{}", FINISH_USAGE);
        } else if (utils::is_prefix_of(command, "frame")) {
            fmt::print("{}", FRAME_USAGE);
        } else if (utils::is_prefix_of(command, "source")) {
            fmt::print("{}", SOURCE_USAGE);
        } else if (utils::is_prefix_of(command, "expression")) {
            fmt::print("{}", EXPRESSION_USAGE);
        } else {
            fmt::print("{}", USAGE);
        }
    }

    void HandleCommand(std::string_view command) {
        auto main_command_offset = command.find(' ');
        std::string_view main_command;
        if (main_command_offset == command.npos) {
            main_command = command;
            command = "";
        } else {
            main_command = command.substr(0, main_command_offset);
            command = command.substr(main_command.size() + 1);
        }
        if (main_command == "") {
            fmt::print("{}", USAGE);
        } else if (utils::is_prefix_of(main_command, "run")) {
            ExitProcess();
            HandleRun(command);
            return;
        } else if (utils::is_prefix_of(main_command, "attach")) {
            ExitProcess();
            Attach(command);
            is_running = true;
            process.WaitForDebugEvent();
            return;
        } else if (utils::is_prefix_of(main_command, "help")) {
            HandleHelp(command);
            return;
        }
        // ======== Following commands needs active process ===========
        if (!process.Active()) {
            fmt::print("{}", USAGE);
            fmt::print(fmt::emphasis::bold | fg(fmt::color::red),
                       "Use the `run` or `attach` command to run a process first.");
            fmt::print("\n");
            return;
        }
        if (utils::is_prefix_of(main_command, "breakpoint")) {
            HandleBreakpoint(command); 
        } else if (utils::is_prefix_of(main_command, "istep")) {
            HandleStepi(command);
        } else if (utils::is_prefix_of(main_command, "inext")) {
            HandleINext(command);
        } else if (utils::is_prefix_of(main_command, "disassemble")) {
            HandleDisassemble(command);
        } else if (utils::is_prefix_of(main_command, "assemble")) {
            HandleAssemble(command);
        } else if (utils::is_prefix_of(main_command, "continue")) {
            HandleContinue(command);
        } else if (utils::is_prefix_of(main_command, "register")) {
            HandleRegister(command);
        } else if (utils::is_prefix_of(main_command, "memory")) {
            HandleMemory(command);
        } else if (utils::is_prefix_of(main_command, "watchpoint")) {
            HandleWatchpoint(command);
        } else if (utils::is_prefix_of(main_command, "next")) {
            HandleNext(command);
        } else if (utils::is_prefix_of(main_command, "step")) {
            HandleStep(command);
        } else if (utils::is_prefix_of(main_command, "finish")) {
            HandleFinish(command);
        } else if (utils::is_prefix_of(main_command, "frame")) {
            HandleFrame(command);
        } else if (utils::is_prefix_of(main_command, "source")) {
            HandleSource(command);
        } else if (utils::is_prefix_of(main_command, "expression")
                || utils::is_prefix_of(main_command, "print")) {
            HandleExpression(command);
        } else {
            fmt::print("{}", USAGE);
        }
    }

    void HandleRun(std::string_view command) {
        auto subcommands = utils::split_v(command);

        if (!fname) {
            fmt::print("No file name was provided, provide the file name as argument at startup");
            return;
        }
        auto [source, program] = ParseProgram(std::string{*fname});

        size_t reg_count = 8;
        auto v = ParseOptionalCommand<size_t>(subcommands, "--reg-count=");
        if (v) {
            reg_count = *v;
        }
        size_t float_reg_count = 4;
        v = ParseOptionalCommand<size_t>(subcommands, "--float-reg-count=");
        if (v) {
            float_reg_count = *v;
        }
        size_t memory_size = 1024;
        v = ParseOptionalCommand<size_t>(subcommands, "--data-size=");
        if (v) {
            memory_size = *v;
        }

        auto m1 = std::make_unique<ThreadMessengerOwner>();
        auto m2 = std::make_unique<ThreadMessenger>(m1->GetOutQueue(), m1->GetInQueue());

        t86vm = std::thread([](std::unique_ptr<ThreadMessenger> messenger,
                           tiny::t86::Program program, size_t reg_cnt,
                           double float_reg_cnt, size_t memory_size) {
            tiny::t86::OS os(reg_cnt, float_reg_cnt, memory_size);
            os.SetDebuggerComms(std::move(messenger));
            os.Run(std::move(program));
        }, std::move(m2), std::move(program), reg_count, float_reg_count, memory_size);
        
        auto t86dbg = std::make_unique<T86Process>(std::move(m1), reg_count,
                                                   float_reg_count, memory_size);

        // This is valid even if process is not yet running.
        auto bkpts = process.GetBreakpoints();
        auto wtchpts = process.GetWatchpoints();

        // Set those guys at the end in case something fails mid-way.
        process = Native(std::move(t86dbg));
        this->source = std::move(source);

        is_running = true;
        process.WaitForDebugEvent();
        process.SetAllBreakpoints(std::move(bkpts));
        process.SetAllWatchpoints(std::move(wtchpts));
        fmt::print("Started process '{}'\n", *fname);
    }


    // Cleanly exits the underlying VM if running.
    void ExitProcess() {
        if (process.Active()) {
            process.Terminate();
            if (t86vm.joinable()) {
                t86vm.join();
            }
        }
    }

    int Run() {
        int exit_code = 0;
        linenoiseHistorySetMaxLen(100);
        char* line_raw;
        const char* prompt = "> ";
        // For testing purposes
        if (std::getenv("NODBGPROMPT")) {
            prompt = "";
        }
        while((line_raw = linenoise(prompt)) != NULL) {
            std::string line = utils::squash_strip_whitespace(line_raw);
            if (line == "") {
                goto FREE_LINE;
            }
            linenoiseHistoryAdd(line_raw);
            try {
                HandleCommand(line);
            } catch (const DebuggerError& err) {
                fmt::print("Error: {}\n", err.what());
            }
FREE_LINE:
            free(line_raw);
        }
        ExitProcess();
        return exit_code;
    }
private:
    /// Checks if there is a command with `searched_command`
    /// prefix and return the suffix.
    /// ie. use for `--reg-count=<This-is-returned>`.
    template<typename T>
    std::optional<T> ParseOptionalCommand(const std::vector<std::string_view>& commands,
                                       std::string_view searched_command) {
        auto f_it = std::find_if(commands.begin(), commands.end(),
                                 [&](auto&& s) { return s.starts_with(searched_command); });
        if (f_it != commands.end()) {
            auto val = f_it->substr(searched_command.size());
            auto opt = utils::svtonum<size_t>(val);
            if (opt) {
                return *opt;
            }
        }
        return {};
    }

    SourceFile ParseSourceFile(std::ifstream& ifs) {
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        return buffer.str();
    }

    void Attach(std::string_view command) {
        auto subcommands = utils::split_v(command);
        if (subcommands.size() < 1) {
            fmt::print("A port is needed, use `attach <port>`");
            return;
        }
        auto port_o = utils::svtonum<int>(subcommands.at(0));
        if (!port_o) {
            fmt::print("Expected port, got '{}'", *port_o);
        }
        auto debuggee = Native::Initialize(*port_o);
        process = Native(std::move(debuggee));
        if (fname) {
            std::ifstream file(*fname);
            if (!file) {
                Error("Unable to open file '{}'\n", *fname);
            }
            source = ParseDebugInfo(file);
        }
    }

    Source ParseDebugInfo(std::ifstream& file) {
        Source source;
        if (file) {
            dbg::Parser p(file);
            auto debug_info = p.Parse();
            if (debug_info.line_mapping) {
                log_info("Found line mapping in debug info");
                source.RegisterLineMapping(std::move(*debug_info.line_mapping));
            }
            if (debug_info.source_code) {
                log_info("Found source code in debug info");
                source.RegisterSourceFile(std::move(*debug_info.source_code));
            }
            if (debug_info.top_die) {
                log_info("Found DIE information in debug info");
                source.RegisterDebuggingInformation(std::move(*debug_info.top_die));
            }
        }
        return source;
    }

    std::pair<Source, tiny::t86::Program> ParseProgram(const std::string& path) {
        std::ifstream file(path);
        if (!file) {
            Error("Unable to open file '{}'\n", *fname);
        }
        Parser parser(file);
        auto program = parser.Parse();
        // Parse debug info
        file.clear();
        file.seekg(0);
        auto source = ParseDebugInfo(file);
        return {std::move(source), std::move(program)};
    }

    std::optional<std::string> fname;

    Native process;
    Source source;
    
    // If the debugger spawned the VM,
    std::thread t86vm;
    
    bool is_running{true};
};
