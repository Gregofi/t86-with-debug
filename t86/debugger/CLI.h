#pragma once

#include <iostream>
#include <string_view>
#include <cstdlib>
#include <cassert>

#include "common/TCP.h"
#include "common/helpers.h"
#include "common/logger.h"
#include "utility/linenoise.h"
#include "Native.h"

class CLI {
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

    void HandleBreakpoint(const std::vector<std::string>& command) {
        // Breakpoint on raw address
        if (command.at(1) == "set-address") {
            auto loc = std::stoull(command.at(2));
            process.SetBreakpoint(loc);
        // Breakpoint on line
        } else if (command.at(1) == "set") {
            NOT_IMPLEMENTED;
        } else if (command.at(1) == "disable-addr") {
            auto loc = std::stoull(command.at(2));
            process.DisableSoftwareBreakpoint(loc);
        } else if (command.at(1) == "enable") {
            
        } else if (command.at(1) == "remove") {
        } else {
            fmt::print(stderr, "Unknown command for breakpoint");
            return;
        }
    }

    void HandleStepi(const std::vector<std::string>& command) {
        if (command.size() == 1) {
            process.PerformSingleStep();
        } else {
            fmt::print(stderr, "Unknown command for breakpoint");
            return;
        }
    }

    void HandleCommand(const std::vector<std::string>& command) {
        if (command.at(0).starts_with("breakpoint")) {
            HandleBreakpoint(command); 
        } else if (command.at(0).starts_with("stepi")) {
            HandleStepi(command);
        } else if (command.at(0).starts_with("disassemble")) {
            if (command.size() == 1) {
                auto ip = process.GetIP();
                PrettyPrintText(ip);
            }
        } else if (command.at(0).starts_with("continue")) {
            process.ContinueExecution();
            process.WaitForDebugEvent();
        } else {
            fmt::print(stderr, "Unknown command '{}'\n", command.at(0));
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
            // The lines are so short we don't really
            // care that we copy it right after
            // and the danger of not freeing is not
            // worth the hassle.
            std::string line{line_raw};
            free(line_raw);
            if (line == "") {
                continue;
            }
            linenoiseHistoryAdd(line.c_str());

            auto command_bits = utils::split(line, ' ');
            HandleCommand(command_bits);
        }
        return 0;
    }
private:
    Native process;

    static const int DEFAULT_DBG_PORT = 9110;
};
