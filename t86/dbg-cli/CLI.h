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
Example: `breakpoint set-addr 1` sets breakpoint at address 1.

For help, use the `<command> help` syntax, for example
`breakpoint help`, or `disassemble help`.

Every command has its subcommands unless explicitly specified.
Some of the commands work with or without subcommands.

commands:
- continue = Continues execution, has no subcommands.
- istep = Assembly level stepping.
- step = Source level stepping.
- disassemble = Disassemble the underlying native code.
- assemble = Rewrite the underlying native code.
- breakpoint = Add and remove breakpoints.
- register = Read and write to registers.
- run = Run the program, has no subcommands.
- attach <port> = Attach to an already running VM, has no subcommands.
- frame = Print information about current function and variables, has no subcommads.
)";
// Do not append anything other than commands here because
// the Cli class will add its own command after this string

    static constexpr const char* BP_USAGE = 
R"(breakpoint <subcommads> [parameter [parameter...]]
If you want to set breakpoint at a specific address, the commands are 
equivalent but with the 'i' prefix, for example to set a breakpoint at
address 1 you would use `breakpoint iset 1` or just `br is 1`.

- set <line> = Creates breakpoint at <line> and enables it.
- unset <line> = Removes breakpoint at <line>.
- enable <line> = Enables breakpoint at <line>.
- disable <line> = Disables breakpoint at <line>.
- list = Lists existing breakpoints.
- help = Print this.
)";
    static constexpr const char* STEPI_USAGE =
R"(istep <subcommands> [parameter [parameter...]]
Assembly level step in.

Without any subcommands (just istep) = Performs instruction level single step.
- help = Print this.
)";
    static constexpr const char* INEXT_USAGE =
R"(istep <subcommands> [parameter [parameter...]]
Assembly level step over.

Without any subcommands (just inext) = Performs instruction level step over.
- help = Print this.
)";

    static constexpr const char* STEP_USAGE =
R"(step <subcommands> [parameter [parameter...]]
Source level step in.

Without any subcommands (just `step`) performs source level single step.
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
- get <begin-addr> <end-addr> - Read from memory range <begin-addr>:<end-addr>.
- gets <begin-addr> <end-addr> - Read from memory range <begin-addr>:<end-addr>,
                                 interpret the result as a c-string. If the string
                                 is missing newline at the end it is appended and
                                 together with a '\%' sequence.
- set <addr> <value> [<value>...] - Write values, beginning at address <addr>.
- sets <addr> <string> - Write string encoded in ASCII on address <addr>. The
                         terminating zero is appended automatically. The string
                         is in form "Hello, World!\n", it must begin and end with
                         quotation marks, they however needn't be escaped inside
                         the string itself.
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
    static constexpr const char* VARIABLE_USAGE =
R"(variable <subcommands> [parameter [parameter...]]
Manipulate with source variables.

commands:
- set <var> <value> - Set a variable <var> to <val>.
- get <var> - Display variable value.
- scope - List all variables in current scope.
)";
    static constexpr const char* PRINT_USAGE =
R"(print <expression>
Evaluate a TinyC expression and return corresponding value.
If you have a variable 'a' and 'b' in scope, you can write
'a + b' to get their sum. If one of those is a pointer, you
can do '*a + b', or if struct: 'a.foo + b'. You can do almost
everything that you can in TinyC.
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
                    PrintVariableValue(varname);
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
                pretty_rest = fmt::format(fg(fmt::color::dark_blue), "->{:>4}:{}\n", i, *line);
            } else {
                pretty_rest = fmt::format("  {:>4}:{}\n", i, *line);
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
                    return fmt::format("Software breakpoint hit at line {}", *line);
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
            uint64_t addr = source.GetAddressFromString(subcommands.at(1));
            process.SetBreakpoint(addr);
            auto line = source.AddrToLine(addr);
            assert(line);
            auto source_line = source.GetLines(*line, 1);
            fmt::print("Breakpoint set on line {} (addr {})", *line, addr);
            if (source_line.size() > 0) {
                fmt::print(": {}\n", source_line[0]);
            } else {
                fmt::print("\n");
            }
        } else if (check_command(subcommands, "remove", 2)) {
            uint64_t addr = source.GetAddressFromString(subcommands.at(1));
            process.UnsetBreakpoint(addr);
            auto line = source.AddrToLine(addr);
            assert(line);
            auto source_line = source.GetLines(*line, 1);
            fmt::print("Breakpoint removed from line {} (addr {})", *line, addr);
            if (source_line.size() > 0) {
                fmt::print(": {}\n", source_line[0]);
            } else {
                fmt::print("\n");
            }
        } else if (check_command(subcommands, "enable", 2)) {
            uint64_t addr = source.GetAddressFromString(subcommands.at(1));
            process.EnableSoftwareBreakpoint(addr);
            auto line = source.AddrToLine(addr);
            assert(line);
            auto source_line = source.GetLines(*line, 1);
            fmt::print("Breakpoint enabled on line {} (addr {})", *line, addr);
            if (source_line.size() > 0) {
                fmt::print(": {}\n", source_line[0]);
            } else {
                fmt::print("\n");
            }
        } else if (check_command(subcommands, "disable", 2)) {
            uint64_t addr = source.GetAddressFromString(subcommands.at(1));
            process.DisableSoftwareBreakpoint(addr);
            auto line = source.AddrToLine(addr);
            assert(line);
            auto source_line = source.GetLines(*line, 1);
            fmt::print("Breakpoint disabled on line {} (addr {})", *line, addr);
            if (source_line.size() > 0) {
                fmt::print(": {}\n", source_line[0]);
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
        } else if (check_command(subcommands, "help", 1)) {
            fmt::print("{}", BP_USAGE); 
        } else {
            fmt::print("{}", BP_USAGE); 
        }
    }

    /// Fetches the value at provided location as bytes.
    uint64_t FetchRawValue(const expr::Location& loc) {
        return std::visit(utils::overloaded {
            [&](const expr::Register& reg) {
                return process.GetRegister(reg.name);
            },
            [&](const expr::Offset& offset) {
                return process.ReadMemory(offset.value, 1).at(0);
            }
        }, loc);
    }

    expr::Location OffsetLocation(const expr::Location& loc, int64_t offset) {
        return ExpressionInterpreter::Interpret({
                expr::Push{loc},
                expr::Push{expr::Offset{offset}},
                expr::Add{}}, process);
    }

    /// Converts structured type value to string.
    std::string StructuredTypeVal(const expr::Location& loc, const Type& type,
                                  const StructuredType& t) {
        std::vector<std::string> res;
        std::ranges::transform(t.members, std::back_inserter(res),
                [&](auto&& m) {
            std::string type = "unknown type";
            std::string value = "?";
            if (m.type) {
                type = fmt::format("{}", fmt::styled(TypeToString(*m.type),
                        fmt::fg(fmt::color::dark_blue)));
                auto true_location = OffsetLocation(loc, m.offset);
                value = TypedVarToStringT86(true_location, *m.type);
            }
            return fmt::format("{}: {} = {}", m.name, type, value);
        });
        return fmt::format("{{{}}}", utils::join(res.begin(), res.end(), ", "));
    }

    /// Special function to print typed variables for T86 because its memory
    /// has 64-bit cells. Ie. if type is of size 1 it actually has 64-bits.
    std::string TypedVarToStringT86(const expr::Location& loc,
                                    const Type& type) {
        auto var_val = std::visit(utils::overloaded {
            [&](const PrimitiveType& t) {
                if (t.size != 1) {
                    Error("Only primitive types with size one are supported");
                }
                auto raw_value = FetchRawValue(loc);
                if (t.type == PrimitiveType::Type::FLOAT) {
                    return fmt::format("{}", static_cast<double>(raw_value));
                } else if (t.type == PrimitiveType::Type::SIGNED) {
                    return fmt::format("{}", static_cast<int64_t>(raw_value));
                } else if (t.type == PrimitiveType::Type::UNSIGNED) {
                    return fmt::format("{}", raw_value);
                } else if (t.type == PrimitiveType::Type::BOOL) {
                    return fmt::format("{}", raw_value > 0 ? "true" : "false");
                } else {
                    UNREACHABLE;
                }
            },
            [&](const PointerType& t) {
                if (t.size != 1) {
                    Error("Only pointers with size one are supported");
                }
                auto raw_value = FetchRawValue(loc);
                return fmt::format("{}", raw_value);
            },
            [&](const StructuredType& t) {
                return StructuredTypeVal(loc, type, t);
            },
            [](auto&&) -> std::string {
                Error("unknown type");
                UNREACHABLE;
            },
        }, type);
        return var_val;
    }

    void SetVariableT86(const expr::Location& location, int64_t value) {
        std::visit(utils::overloaded {
            [&](const expr::Register& r) {
                process.SetRegister(r.name, value);
            },
            [&](const expr::Offset& m) {
                process.SetMemory(m.value, {value});
            },
        }, location);
    }

    void PrintVariableValue(std::string_view name) {
        auto location = source.GetVariableLocation(process, name);
        if (!location) {
            Error("Variable '{}' is not in scope or missing debug info", name);
        }
        auto type_info = source.GetVariableTypeInformation(process, name);
        if (!type_info) {
            fmt::print("No type information about variable '{}'.\n", name);
            auto raw_value = FetchRawValue(*location);
            fmt::print("Raw byte value: ({}) {} = {}\n",
                    LocationToStr(*location), name, raw_value);
        } else if (Arch::GetMachine() == Arch::Machine::T86) {
            auto var_val = TypedVarToStringT86(*location, *type_info);
            fmt::print("({}; {}) {} = {}\n", expr::LocationToStr(*location),
                    TypeToString(*type_info), name, var_val);
        } else {
            Error("Printing typed variables is not supported in current architecture");
        }
    }

    void SetVariableValue(std::string_view name, int64_t value_raw) {
        auto location = source.GetVariableLocation(process, name);
        if (!location) {
            Error("Variable '{}' is not in scope or missing debug info", name);
        }
        if (Arch::GetMachine() == Arch::Machine::T86) {
            SetVariableT86(*location, value_raw);    
        }
    }

    void HandleVariable(std::string_view command) {
        auto subcommands = utils::split_v(command);
        if (check_command(subcommands, "get", 2)) {
            auto var_name = subcommands.at(1);
            PrintVariableValue(var_name);
        } else if (check_command(subcommands, "set", 3)) {
            auto var_name = subcommands.at(1);
            // TODO: Should, at the very least, handle other primitive types.
            auto value = utils::svtonum<int64_t>(subcommands.at(2));
            if (!value) {
                Error("Expected number, got '{}'", subcommands.at(2));
            }
            SetVariableValue(var_name, *value);
        } else {
            fmt::print(VARIABLE_USAGE);
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

    void HandleStep(std::string_view command) {
        if (!process.Active()) {
            Error("No active process.");
        }
        if (!is_running) {
            Error("Process finished executing, it's not possible to continue.");
        }
        if (command == "") {
            auto e = source.StepIn(process);
            if (!std::holds_alternative<Singlestep>(e)) {
                ReportDebugEvent(e);
            }
            if (std::holds_alternative<ExecutionEnd>(e)) {
                is_running = false;
            } else {
                auto ip = process.GetIP();
                auto line = source.AddrToLine(ip);
                // The line must be valid here since we did a source level singlestep.
                assert(line);
                PrettyPrintCode(*line);
            }
        } else {
            fmt::print("{}", STEP_USAGE);
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
        } else if (utils::is_prefix_of(command, "help")) {
            fmt::print("{}", STEPI_USAGE);
        } else {
            fmt::print("{}", STEPI_USAGE);
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
        } else if (check_command(subcommands, "help", 1)) {
            fmt::print("{}", DISASSEMBLE_USAGE);
        } else {
            fmt::print("{}", DISASSEMBLE_USAGE);
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

    void HandlePrint(std::string_view command) {
        if (!process.Active()) {
            Error("No active process.");
        }
        if (command == "") {
            fmt::print("{}", PRINT_USAGE);
            return;
        }
        auto val = source.EvaluateExpression(process, std::string{command});
        fmt::print("{}\n", TypedValueToString(val));
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
        if (utils::is_prefix_of(main_command, "run")) {
            ExitProcess();
            HandleRun(command);
            return;
        } else if (utils::is_prefix_of(main_command, "attach")) {
            ExitProcess();
            Attach(command);
            is_running = true;
            process.WaitForDebugEvent();
            return;
        }
        // Following commands needs active process
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
        } else if (utils::is_prefix_of(main_command, "variable")) {
            HandleVariable(command);
        } else if (utils::is_prefix_of(main_command, "watchpoint")) {
            HandleWatchpoint(command);
        } else if (utils::is_prefix_of(main_command, "step")) {
            HandleStep(command);
        } else if (utils::is_prefix_of(main_command, "frame")) {
            HandleFrame(command);
        } else if (utils::is_prefix_of(main_command, "print")) {
            HandlePrint(command);
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
        auto f_it = std::ranges::find_if(commands, [&](auto&& s) { return s.starts_with(searched_command); });
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
    }

    std::pair<Source, tiny::t86::Program> ParseProgram(const std::string& path) {
        std::ifstream file(path);
        if (!file) {
            Error("Unable to open file '{}'\n", *fname);
        }
        Parser parser(file);
        auto program = parser.Parse();
        // Parse debug info
        Source source;
        file.clear();
        file.seekg(0);
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
        return {std::move(source), std::move(program)};
    }

    std::optional<std::string> fname;

    Native process;
    Source source;
    
    // If the debugger spawned the VM,
    std::thread t86vm;
    
    bool is_running{true};
};
