#include "t86/debug.h"

namespace tiny::t86 {
    std::string Debug::ReasonToString(BreakReason reason) {
        switch (reason) {
            case BreakReason::Begin: return "START";
            case BreakReason::SoftwareBreakpoint: return "SW_BKPT";
            case BreakReason::HardwareBreakpoint: return "HW_BKPT";
            case BreakReason::SingleStep: return "SINGLESTEP";
            case BreakReason::Halt: return "HALT";
            case BreakReason::CpuError: return "CPU_ERROR";
        }
        UNREACHABLE;
    }

    size_t Debug::svtoidx(std::string_view s) {
        auto v = utils::svtoi64(s);
        if (!v || *v < 0) {
            throw std::runtime_error(fmt::format("Expected index, got '{}' {}", s, !v));
        }
        return static_cast<size_t>(*v);
    }

    Register Debug::TranslateToRegister(std::string_view s) {
        if (s == "IP") {
            return Register::ProgramCounter();
        } else if (s == "BP") {
            return Register::StackBasePointer();
        } else if (s == "SP") {
            return Register::StackPointer();
        } else if (s == "FLAGS") {
            return Register::Flags();
        } 
        auto idx = utils::svtonum<size_t>(s.substr(1));
        if (!idx) {
            throw std::runtime_error(
                    fmt::format("TranslateToRegister failed, got '{}' as index",
                        s.substr(1)));
        }
        log_debug("Register index: {}",* idx);
        return Register(*idx);
    }

    FloatRegister Debug::TranslateToFloatRegister(std::string_view s) {
        auto idx = utils::svtonum<size_t>(s.substr(1));
        if (!idx) {
            throw std::runtime_error(
                    fmt::format("TranslateToRegister failed, got '{}' as index",
                        s.substr(1)));
        }
        log_debug("Float register index: {}",* idx);
        return FloatRegister(*idx);
    }

    size_t Debug::TranslateToDebugRegister(std::string_view s) {
        auto idx = utils::svtonum<size_t>(s.substr(1));
        if (!idx) {
            throw std::runtime_error(fmt::format("Wrong debug register,"
                       " expected D<index>, got '{}'", s));
        }
        return *idx;
    }

    std::unique_ptr<Instruction> Debug::ParseInstruction(std::string_view s) {
        std::istringstream is{std::string(s)};
        Parser parser(is);
        auto ins = parser.Instruction();
        return ins;
    }

    std::string Debug::FloatRegistersToString() const {
        std::string acc;
        // Float registers
        size_t float_reg_cnt = cpu.floatRegistersCount();
        for (size_t i = 0; i < float_reg_cnt; ++i) {
            acc += fmt::format("F{}:{}\n", i, cpu.getFloatRegister(FloatRegister{i}));
        }
        return acc; 
    }

    std::string Debug::RegistersToString() const {
        std::string acc;
        acc += fmt::format("IP:{}\n", cpu.getRegister(Register::ProgramCounter()));
        acc += fmt::format("BP:{}\n", cpu.getRegister(Register::StackBasePointer()));
        acc += fmt::format("SP:{}\n", cpu.getRegister(Register::StackPointer()));
        // Normal registers
        size_t reg_cnt = cpu.registersCount();
        for (size_t i = 0; i < reg_cnt; ++i) {
            acc += fmt::format("R{}:{}\n", i, cpu.getRegister(Register{i}));
        }
        return acc;
    }

    std::string Debug::DebugRegistersToString() const {
        std::string acc;
        for (size_t i = 0; i < Cpu::DEBUG_REGISTERS_CNT; ++i) {
            acc += fmt::format("D{}:{}\n", i, cpu.getDebugRegister(i));
        }
        return acc;
    }

    /// Use to pass control to the debug interface
    /// which will communicate with the client.
    /// Should be called on any break situation.
    bool Debug::Work(BreakReason reason) {
        if (reason == BreakReason::SingleStep) {
            log_info("Debug handler: After singlestep");
            cpu.unsetTrapFlag();
        } 
        log_info("Sending stop message to the debugger");
        messenger->Send("STOPPED");
        
        while (true) {
            log_info("Waiting for message from the debugger");
            auto message = messenger->Receive();
            // TODO: This means that we received EOF, which shouldn't
            //       really happen.
            if (!message) {
                return false;
            }
            log_info("Received message '{}' from debugger", *message);
            auto commands = utils::split_v(*message, ' ');
            auto command = commands[0];
            if (command == "REASON") {
                std::string r = ReasonToString(reason);
                messenger->Send(r);
            } else if (message == "CONTINUE") {
                messenger->Send("OK");
                break;
            } else if (command.starts_with("PEEKTEXT")) {
                auto index = svtoidx(commands.at(1));
                auto count = svtoidx(commands.at(2));
                std::string result;
                for (size_t i = index; i < index + count; ++i) {
                    result += cpu.getText(i).toString() + "\n";
                }
                messenger->Send(result);
            } else if (command.starts_with("POKETEXT")) {
                // We unfortunately broke the instruction into several parts,
                // need to glue it back together
                auto index = svtoidx(commands.at(1));
                // Glue the operands together
                auto insBegin = std::next(commands.begin(), 3);
                // The commas are already included
                auto operands = utils::join(insBegin, commands.end(), " ");

                auto ins_s = std::string(commands.at(2)) + " " + operands;
                log_info("Setting instruction '{}' at address {}", ins_s, index);
                auto ins = ParseInstruction(ins_s);
                cpu.setText(index, std::move(ins));
                messenger->Send("OK");
            } else if (command.starts_with("PEEKDATA")) {
                auto index = svtoidx(commands.at(1));
                auto count = svtoidx(commands.at(2));
                std::string result;
                for (size_t i = index; i < index + count; ++i) {
                    result += fmt::format("{}\n", cpu.getMemory(i)); 
                }
                messenger->Send(result);
            } else if (command.starts_with("POKEDATA")) {
                auto index = svtoidx(commands.at(1));
                auto value = utils::svtoi64(commands.at(2));
                if (!value) {
                    throw std::runtime_error("Expected number as second arg");
                }
                cpu.setMemory(index, *value);
                messenger->Send("OK");
            } else if (command == "PEEKREGS") {
                auto regs = RegistersToString();
                messenger->Send(regs);
            } else if (command == "PEEKFLOATREGS") {
                auto regs = FloatRegistersToString();
                messenger->Send(regs);
            } else if (command == "PEEKDEBUGREGS") {
                auto regs = DebugRegistersToString();
                messenger->Send(regs);
            } else if (command == "POKEDEBUGREGS") {
                auto reg = TranslateToDebugRegister(commands.at(1));
                auto val = *utils::svtonum<uint64_t>(commands.at(2));
                cpu.setDebugRegister(reg, val);
                messenger->Send("OK");
            } else if (command.starts_with("POKEFLOATREGS")) {
                auto reg = TranslateToFloatRegister(commands.at(1));
                auto val = *utils::svtonum<double>(commands.at(2));
                cpu.setFloatRegisterDebug(reg, val);
                messenger->Send("OK");
            } else if (command.starts_with("POKEREGS")) {
                auto reg = TranslateToRegister(commands.at(1));
                auto val = *utils::svtonum<int64_t>(commands.at(2));
                cpu.setRegisterDebug(reg, val);
                messenger->Send("OK");
            } else if (command == "SINGLESTEP") {
                cpu.setTrapFlag();
                messenger->Send("OK");
                break; // continue
            } else if (command == "REGCOUNT") {
                messenger->Send(fmt::format(
                    "REGCOUNT:{}", Cpu::Config::instance().registerCnt()));
            } else if (command == "TEXTSIZE") {
                messenger->Send(fmt::format("TEXTSIZE:{}", cpu.textSize()));
            } else if (command == "DATASIZE") {
                messenger->Send(fmt::format("DATASIZE:{}",
                                            Cpu::Config::instance().ramSize()));
            } else if (command == "TERMINATE") {
                messenger->Send("OK");
                return false;
            } else {
                messenger->Send("UNKNOWN COMMAND");
                continue;
            }
        }

        return true;
    }
}
