#pragma once
#include "common/TCP.h"
#include "common/logger.h"
#include "common/helpers.h"
#include "t86/cpu/register.h"
#include "t86/cpu.h"

namespace tiny::t86 {

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

    Register TranslateToRegister(std::string_view s) {
        if (s == "IP") {
            return Register::ProgramCounter();
        } else if (s == "BP") {
            return Register::StackBasePointer();
        } else if (s == "SP") {
            return Register::StackPointer();
        } else if (s == "FLAGS") {
            return Register::Flags();
        } else {
            NOT_IMPLEMENTED;
        }
    }

    /// Use to pass control to the debug interface
    /// which will communicate with the client.
    /// Should be called on any break situation.
    bool Work(BreakReason reason) {
        log_info("Sending stop message to the debugger");
        server.Send("Program stopped");
        while (true) {
            log_info("Waiting for message from the debugger");
            auto message = server.Receive();
            // TODO: This means that we received EOF, which shouldn't
            //       really happen.
            if (!message) {
                return false;
            }
            log_info("Received message '{}' from debugger", *message);
            auto commands = utils::split(*message, ' ');
            auto command = commands[0];
            if (command == "REASON") {
                std::string r = ReasonToString(reason);
                server.Send(r);
            } else if (message == "CONTINUE") {
                server.Send("Ok");
                break;
            } else if (command.starts_with("PEEKTEXT")) {
            } else if (command.starts_with("POKETEXT")) {
            } else if (command.starts_with("PEEKDATA")) {
            } else if (command.starts_with("POKEDATA")) {
            } else if (command.starts_with("PEEKREGS")) {
                auto reg = TranslateToRegister(commands.at(1));
                auto val = cpu.getRegister(reg);
                server.Send(fmt::format("Reg {} value: {}", commands.at(1), val));
            } else if (command.starts_with("POKEREGS")) {
            } else if (command == "SINGLESTEP") {

            } else {
                server.Send("Unknown command");
                continue;
            }
        }

        return true;
    }
private:
    Cpu& cpu;
    TCP::TCPServer server;

};
}
