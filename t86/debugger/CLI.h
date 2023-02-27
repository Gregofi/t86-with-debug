#pragma once

#include <iostream>
#include <string_view>
#include <cstdlib>

#include "common/TCP.h"
#include "common/helpers.h"
#include "common/logger.h"
#include "utility/linenoise.h"
#include "Native.h"

class CLI {
public:
    CLI(): process(DEFAULT_DBG_PORT) {

    }

    /// Dumps text around current IP
    /// @param range - How many instructions to dump above
    ///                and below current IP.
    void dumpText(uint64_t address, int range = 2) {

    }

    void handleBreakpoint(const std::vector<std::string>& command) {
        // Breakpoint on raw address
        if (command.at(1) == "set-address") {
            auto loc = std::stoull(command.at(2));
            auto err = process.SetBreakpoint(loc);
            if (err) {
                fmt::print(stderr, "Error while setting breakpoint: {}", *err);
            } else {
                fmt::print("Breakpoint set at address {}", loc);
            }
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

    void handleStepi(const std::vector<std::string>& command) {
        if (command.at(1) == "single") {
            process.PerformSingleStep();
        } else {
            fmt::print(stderr, "Unknown command for breakpoint");
            return;
        }
    }
    
    int Run() {
        linenoiseHistorySetMaxLen(100);
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
            if (command_bits.at(0).starts_with("breakpoint")) {
                handleBreakpoint(command_bits); 
            } else if (command_bits.at(0).starts_with("stepi")) {
                
            } else {
                fmt::print(stderr, "Unknown command '{}'", command_bits.at(0));
            }
        }
        return 0;
    }
private:
    Native process;

    static const int DEFAULT_DBG_PORT = 9110;
};
