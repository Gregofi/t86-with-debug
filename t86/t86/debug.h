#pragma once
#include <stdexcept>
#include <string>
#include <memory>
#include <span>
#include "common/TCP.h"
#include "common/logger.h"
#include "common/helpers.h"
#include "t86/cpu/register.h"
#include "t86/cpu.h"
#include "common/messenger.h"
#include "common/parser.h"

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

    size_t svtoidx(std::string_view s) {
        auto v = utils::svtoi64(s);
        if (!v || *v < 0) {
            throw std::runtime_error(fmt::format("Expected index, got '{}' {}", s, !v));
        }
        return static_cast<size_t>(*v);
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
            auto idx = svtoidx(s.substr(1));
            log_debug("Register index: {}", idx);
            return Register{static_cast<size_t>(idx)};
        }
    }

    Instruction* ParseInstruction(std::string_view s) {
        std::istringstream is{std::string(s)};
        Parser parser(is);
        Instruction* ins = parser.Instruction();
        return ins;
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
                auto index = svtoidx(commands.at(1));
                const Instruction *ins = cpu.getText(index);
                messenger->Send(fmt::format("INS AT {}: '{}'", index, ins->toString()));
            } else if (command.starts_with("POKETEXT")) {
                // We unfortunately broke the instruction into several parts, need to glue it back together
                auto index = svtoidx(commands.at(1));
                // Glue the operands together
                auto insBegin = std::next(commands.begin(), 3);
                // The commas are already included
                auto operands = utils::join(insBegin, commands.end(), " ");

                auto ins_s = std::string(commands.at(2)) + " " + operands;
                log_info("Setting instruction '{}' at address {}", ins_s, index);
                auto ins = ParseInstruction(ins_s);
                cpu.setText(index, ins);
                messenger->Send("Ok");
            } else if (command.starts_with("PEEKDATA")) {
                auto index = svtoidx(commands.at(1));
                messenger->Send(fmt::format("MEMORY {} VALUE: {}", index, cpu.getMemory(index)));
            } else if (command.starts_with("POKEDATA")) {
                auto index = svtoidx(commands.at(1));
                auto value = utils::svtoi64(commands.at(2));
                if (!value) {
                    throw std::runtime_error("Expected number as second arg");
                }
                cpu.setMemory(index, *value);
                messenger->Send("Ok");
            } else if (command.starts_with("PEEKREGS")) {
                auto reg = TranslateToRegister(commands.at(1));
                auto val = cpu.getRegister(reg);
                messenger->Send(fmt::format("Reg {} value: {}", commands.at(1), val));
            } else if (command.starts_with("POKEREGS")) {
                auto reg = TranslateToRegister(commands.at(1));
                auto val = svtoidx(commands.at(2));
                cpu.setRegisterDebug(reg, val);
                messenger->Send("Ok");
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
