#pragma once
#include "common/TCP.h"
#include "common/logger.h"

namespace tiny::t86 {
class Cpu;

/// A debugging interface provided by the VM.
class Debug {
public:
    Debug(int port, Cpu& cpu): cpu(cpu), server(port) {}

    enum class BreakReason {
        Begin,
        SoftwareBreakpoint,
        HardwareBreakpoint,
        SingleStep,
        Halt,
    };

    /// Opens connection at specified port and
    /// waits for debugger to connect.
    /// This call is blocking.
    void OpenConnection() {
        server.Initialize();
    }

    std::string ReasonToString(BreakReason reason) {
        switch (reason) {
            case BreakReason::Begin: return "Execution start";
            case BreakReason::SoftwareBreakpoint: return "Software breakpoint";
            case BreakReason::HardwareBreakpoint: return "Hardware breakpoint";
            case BreakReason::SingleStep: return "Single step";
            case BreakReason::Halt: return "Halt";
        }
    }

    /// Use to pass control to the debug interface
    /// which will communicate with the client.
    /// Should be called on any break situation.
    bool Work(BreakReason reason) {
        log_info("Sending stop message to the debugger");
        server.Send("Program stopped");
        bool end = false;
        while (!end) {
            log_info("Waiting for message from the debugger");
            auto message = server.Receive();
            // TODO: This means that we received EOF, which shouldn't
            //       really happen.
            if (!message) {
                return false;
            }
            log_info("Received message '{}' from debugger", *message);
            
            if (message == "REASON") {
                std::string r = ReasonToString(reason);
                server.Send(r);
            } else if (message == "CONTINUE") {
                end = true;
            } else {
                server.Send("Unknown command");
                continue;
            }
            server.Send("Ok");
        }

        return true;
    }
private:
    Cpu& cpu;
    TCP::TCPServer server;

};
}
