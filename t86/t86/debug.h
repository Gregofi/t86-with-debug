#pragma once
#include <string>
#include <memory>
#include "common/TCP.h"
#include "common/logger.h"
#include "common/helpers.h"
#include "t86/cpu/register.h"
#include "t86/cpu.h"
#include "common/messenger.h"

namespace tiny::t86 {

/// A debugging interface provided by the VM.
class Debug {
public:
    Debug(Cpu& cpu, std::unique_ptr<Messenger> mess): cpu(cpu), messenger(std::move(mess)) {}

    enum class BreakReason {
        Begin,
        SoftwareBreakpoint,
        HardwareBreakpoint,
        SingleStep,
        Halt,
    };

    std::string ReasonToString(BreakReason reason) {
        switch (reason) {
            case BreakReason::Begin: return "START";
            case BreakReason::SoftwareBreakpoint: return "SW_BKPT";
            case BreakReason::HardwareBreakpoint: return "HW_BKPT";
            case BreakReason::SingleStep: return "SINGLE_STEP";
            case BreakReason::Halt: return "HALT";
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
        if (reason == BreakReason::SingleStep) {
            log_info("Debug handler: After singlestep");
            cpu.unsetTrapFlag();
        } else {
            log_info("Sending stop message to the debugger");
            messenger->Send("Program stopped");
        }
        while (true) {
            log_info("Waiting for message from the debugger");
            auto message = messenger->Receive();
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
                messenger->Send(r);
            } else if (message == "CONTINUE") {
                messenger->Send("OK");
                break;
            } else if (command.starts_with("PEEKTEXT")) {
                NOT_IMPLEMENTED;
            } else if (command.starts_with("POKETEXT")) {
                NOT_IMPLEMENTED;
            } else if (command.starts_with("PEEKDATA")) {
                NOT_IMPLEMENTED;
            } else if (command.starts_with("POKEDATA")) {
                NOT_IMPLEMENTED;
            } else if (command.starts_with("PEEKREGS")) {
                auto reg = TranslateToRegister(commands.at(1));
                auto val = cpu.getRegister(reg);
                messenger->Send(fmt::format("Reg {} value: {}", commands.at(1), val));
            } else if (command.starts_with("POKEREGS")) {
                auto reg = TranslateToRegister(commands.at(1));
                auto val = utils::svtoi64(commands.at(2));
                if (!val) {
                    throw std::runtime_error("Couldn't convert POKEREGS arg to number");
                }
                cpu.setRegister(reg, *val);
            } else if (command == "SINGLESTEP") {
                cpu.setTrapFlag();
                messenger->Send("OK");
                break; // continue
            } else {
                messenger->Send("Unknown command");
                continue;
            }
        }

        return true;
    }
private:
    Cpu& cpu;
    std::unique_ptr<Messenger> messenger;
};
}
