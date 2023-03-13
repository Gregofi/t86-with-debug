#pragma once

#include <iostream>
#include <fstream>
#include <string_view>
#include <cstdlib>
#include <cassert>

#include "common/TCP.h"
#include "common/helpers.h"
#include "common/logger.h"
#include "debugger/Source/Parser.h"
#include "debugger/Source/Source.h"
#include "t86/os.h"
#include "threads_messenger.h"
#include "utility/linenoise.h"
#include "debugger/Native.h"

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

Every command has its subcommands, except continue,
which should be used alone.

commands:
- continue = Continues execution, has no subcommands.
- istep = Assembly level stepping.
- disassemble = Disassemble the underlying native code.
- assemble = Rewrite the underlying native code.
- breakpoint = Add and remove breakpoints.
- register = Read and write to registers.
- run = Run the program, has no subcommands.
- attach <port> = Attach to an already running VM, has no subcommands.
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
Used for instruction level single stepping

Without any subcommands (just istep) = Performs instruction level single step.
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

public:
    Cli(std::string fname): fname(std::move(fname)) {
    }

    Cli() = default;


    /// Parses string from given stringview, converts most
    /// of the escape characters ('\' + 'n' -> '\n') into
    /// one and checks if it begins and ends with '"'.
    std::string ParseString(std::string_view s) {
        if (s.size() < 2 || !s.starts_with('"') || !s.ends_with('"')) {
            throw DebuggerError("Expected string to begin and end with '\"'");
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
                    throw DebuggerError(fmt::format("Unknown escape sequence '{}{}'",
                                                    prev, s[i]));
                }
                prev = s[++i];
            } else {
                result += prev;
                prev = s[i];
            }
        }
        return result;
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
        for (size_t i = 0; i < inst.size(); ++i) {
            uint64_t curr_addr = i + begin;
            if (curr_addr == address) {
                fmt::print(fg(fmt::color::dark_blue), "-> {}\t{}\n", curr_addr, inst[i]);
            } else {
                fmt::print("   {}\t{}\n", curr_addr, inst[i]);
            }
        }
    }

    /// Prints source code around given line, if no line exists then
    /// nothing is printed.
    void PrettyPrintCode(ssize_t line, int range = 2) {
        auto begin = std::max(line - range, 0l);
        auto end = line + range + 1;
        for (ssize_t i = begin; i < end; ++i) {
            auto code = source.GetLine(i);
            if (code) {
                if (i == line) {
                    fmt::print(fg(fmt::color::dark_blue), "-> {}:{}\n", i, *code);
                } else {
                    fmt::print("   {}:{}\n", i, *code);
                }
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
        }, e);
    }

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
            throw DebuggerError("No active process.");
        }
        // Breakpoint on raw address
        auto subcommands = utils::split_v(command);
        if (check_command(subcommands, "iset", 2)) {
            auto address = ParseAddress(subcommands.at(1));
            auto line = process.ReadText(address, 1);
            process.SetBreakpoint(address);
            fmt::print("Breakpoint set on line {}: '{}'\n", address, line[0]);
        // Breakpoint on line
        } else if (check_command(subcommands, "set", 2)) {
            auto line = utils::svtonum<size_t>(subcommands.at(1));
            if (!line) {
                Error("Expected line, got '{}'", subcommands.at(1));
            }
            auto addr = source.SetSourceSoftwareBreakpoint(process, *line);
            auto source_line = source.GetLines(*line, 1);
            fmt::print("Breakpoint set on line {} (addr {})", *line, addr);
            if (source_line.size() > 0) {
                fmt::print(": {}\n", source_line[0]);
            } else {
                fmt::print("\n");
            }
        } else if (check_command(subcommands, "remove", 2)) {
            auto line = utils::svtonum<size_t>(subcommands.at(1));
            if (!line) {
                Error("Expected line, got '{}'", subcommands.at(1));
            }
            auto addr = source.SetSourceSoftwareBreakpoint(process, *line);
            auto source_line = source.GetLines(*line, 1);
            fmt::print("Breakpoint removed from line {} (addr {})", *line, addr);
            if (source_line.size() > 0) {
                fmt::print(": {}\n", source_line[0]);
            } else {
                fmt::print("\n");
            }
        } else if (check_command(subcommands, "idisable", 2)) {
            auto address = ParseAddress(subcommands.at(1));
            process.DisableSoftwareBreakpoint(address);
        } else if (check_command(subcommands, "ienable", 2)) {
            auto address = ParseAddress(subcommands.at(1));
            process.EnableSoftwareBreakpoint(address);
        } else if (check_command(subcommands, "iremove", 2)) {
            auto address = ParseAddress(subcommands.at(1));
            process.UnsetBreakpoint(address);
        } else if (check_command(subcommands, "help", 1)) {
            fmt::print("{}", BP_USAGE); 
        } else {
            fmt::print("{}", BP_USAGE); 
        }
    }

    void HandleStepi(std::string_view command) {
        if (!process.Active()) {
            throw DebuggerError("No active process.");
        }
        if (!is_running) {
            throw DebuggerError("Process finished executing, it's not possible to continue.");
        }
        if (command == "") {
            auto e = process.PerformSingleStep();
            if (!std::holds_alternative<Singlestep>(e)) {
                fmt::print("Process stopped, reason: {}\n",
                           DebugEventToString(e));
                if (std::holds_alternative<ExecutionEnd>(e)) {
                    is_running = false;
                }
            }
            // Instruction level stepping should always
            // display the instructions even if they
            // have source line mapping.
            auto ip = process.GetIP();
            PrettyPrintText(ip);
        } else if (utils::is_prefix_of(command, "help")) {
            fmt::print("{}", STEPI_USAGE);
        } else {
            fmt::print("{}", STEPI_USAGE);
        }
    }

    void HandleDisassemble(std::string_view command) {
        if (!process.Active()) {
            throw DebuggerError("No active process.");
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

    void InteractiveAssemble(size_t address) {
        char* line_raw;
        // TODO: Save and rollback linenoise history
        std::vector<std::string> result;
        for (size_t i = 0; ;++i) {
            line_raw = linenoise(fmt::format("{}: > ", address + i).c_str());
            if (line_raw == NULL) {
                throw DebuggerError("Unexpected end of input in interactive assembling, "
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
            throw DebuggerError("No active process.");
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
            throw DebuggerError("No active process.");
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
                throw DebuggerError(
                    fmt::format("Expected register value, instead got '{}'",
                                subcommands.at(2)));
            }
            process.SetFloatRegister(std::string(reg), *value);
        } else {
            fmt::print("{}", REGISTER_USAGE);
        }
    }

    void HandleMemory(std::string_view command) {
        if (!process.Active()) {
            throw DebuggerError("No process is running");
        }
        auto subcommands = utils::split_v(command);
        if (check_command(subcommands, "set", 3)) {
            auto cell = utils::svtonum<size_t>(subcommands.at(1));
            std::vector<int64_t> values;
            std::transform(std::next(subcommands.begin(), 2), subcommands.end(), 
                    std::back_inserter(values), [&](auto&& s) { 
                auto v = utils::svtonum<int64_t>(s);
                if (!v) {
                    throw DebuggerError(fmt::format("Expected number, got '{}'", s));
                }
                return *v;
            });
            process.SetMemory(*cell, values);
        } else if (check_command(subcommands, "setstr", 3)) {
            auto cell = utils::svtonum<size_t>(subcommands.at(1));
            if (!cell) {
                throw DebuggerError(fmt::format("Expected memory cell and string"));
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
            auto end = utils::svtonum<size_t>(subcommands.at(2));
            if (!begin || !end || begin > end) {
                throw DebuggerError(fmt::format("Expected range of memory cells, "
                            "instead got '{}', '{}'", subcommands.at(1),
                            subcommands.at(2)));
            }
            auto vals = process.ReadMemory(*begin, *end - *begin + 1);
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

    void HandleContinue(std::string_view command) {
        if (!process.Active()) {
            throw DebuggerError("No active process.");
        }
        if (!is_running) {
            throw DebuggerError("Process finished executing, it's not possible to continue.");
        }
        process.ContinueExecution();
        auto e = process.WaitForDebugEvent();
        if (std::holds_alternative<ExecutionEnd>(e)) {
            is_running = false;
        }
        fmt::print("Process stopped, reason: {}\n", DebugEventToString(e));
        ReportBreak(e);
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
            Run(command);
            is_running = true;
            process.WaitForDebugEvent();
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
            fmt::print("Use the `run` or `attach` command to run a process first.\n");
            return;
        }
        if (utils::is_prefix_of(main_command, "breakpoint")) {
            HandleBreakpoint(command); 
        } else if (utils::is_prefix_of(main_command, "istep")) {
            HandleStepi(command);
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
        } else {
            fmt::print("{}", USAGE);
        }
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
            throw DebuggerError(fmt::format("Unable to open file '{}'\n", *fname));
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

    void Run(std::string_view command) {
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
                           double float_reg_cnt) {
            tiny::t86::OS os(reg_cnt, float_reg_cnt);
            os.SetDebuggerComms(std::move(messenger));
            os.Run(std::move(program));
        }, std::move(m2), std::move(program), reg_count, float_reg_count);
        
        auto t86dbg = std::make_unique<T86Process>(std::move(m1), reg_count,
                                                   float_reg_count);
        // Set those guys at the end in case something fails mid-way.
        process = Native(std::move(t86dbg));
        this->source = std::move(source);
        fmt::print("Started process '{}'\n", *fname);
    }

    std::optional<std::string> fname;

    Native process;
    Source source;
    
    // If the debugger spawned the VM,
    std::thread t86vm;
    
    bool is_running{true};
};
