#pragma once
#include <istream>
#include "common/threads_messenger.h"
#include "t86/os.h"

inline void RunCPU(std::unique_ptr<ThreadMessenger> messenger,
            const std::string &program, size_t register_cnt = 4) {
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    
    tiny::t86::OS os(register_cnt);
    os.SetDebuggerComms(std::move(messenger));
    os.Run(std::move(p));
}
