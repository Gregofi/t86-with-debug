#pragma once
#include <string_view>
#include <vector>
#include <memory>
#include <functional>

#include "common/messenger.h"
#include "common/logger.h"
#include "common/helpers.h"
#include "fmt/core.h"
#include "DebuggerError.h"
#include "DebugEvent.h"
#include "Process.h"

class T86Process: public Process {
public:
    T86Process(std::unique_ptr<Messenger> process, size_t gp_reg_cnt = 10,
               size_t data_size = 1024)
                    : process(std::move(process)),
                      data_size(data_size),
                      gen_purpose_regs_count(gp_reg_cnt) {
    }

  /*
    void InitSizes() {
        // TODO: This isn't right, at least the sizes
        // should be optional and throw if this wasn't
        // called before accessing them.
        process->Send("DATASIZE");
        data_size = std::stoull(process->Receive()->substr(strlen("DATASIZE:")));
        process->Send("REGCOUNT");
        gen_purpose_regs_count =
            std::stoull(process->Receive()->substr(strlen("REGCOUNT:")));
        text_size = TextSize();
    }
*/

    /// Writes into the processes text area, overwriting instructions.
    /// You can only write up to the size of the program.
    /// Caller is responsible that the text size of the program is
    /// respected.
    void WriteText(uint64_t address,
                   const std::vector<std::string> &data) override {
        for (size_t i = 0; i < data.size(); ++i) {
            process->Send(fmt::format("POKETEXT {} {}", address + i, data[i]));
            CheckResponse("POKETEXT error");
        }
    }

    /// Returns range of instructions of length 'amount' starting at 'address'.
    /// Caller is responsible that the size of the program is respected.
    /// Use the TextSize for getting the size of text.
    std::vector<std::string> ReadText(uint64_t address, size_t amount) override {
        process->Send(fmt::format("PEEKTEXT {} {}", address, amount));
        auto text = process->Receive();
        if (!text) {
            throw DebuggerError("PEEKTEXT err");
        }

        return utils::split(*text, '\n');
    }

    void WriteMemory(uint64_t address, std::vector<uint64_t> data) override {
        for (size_t i = 0; i < data.size(); ++i) {
            process->Send(fmt::format("POKEDATA {} {}", address + i, data[i]));
            CheckResponse("POKEDATA error");
        }
    }

    std::vector<uint64_t> ReadMemory(uint64_t address, size_t amount) override {
        process->Send(fmt::format("PEEKDATA {} {}", address, amount));
        auto data = process->Receive();
        if (!data) {
            throw DebuggerError("PEEKDATA err");
        }

        auto splitted = utils::split(*data);
        std::vector<uint64_t> result;
        std::transform(splitted.begin(), splitted.end(), std::back_inserter(result),
                [](auto&& s) { return std::stoull(s); });
        return result;
    }

    DebugEvent GetReason() override {
        process->Send("REASON");
        auto reason_opt = process->Receive();
        if (!reason_opt) {
            throw DebuggerError("REASON error");
        }
        auto &r = *reason_opt;
        if (r == "START") {
            return DebugEvent::ExecutionBegin;
        } else if (r == "SW_BKPT") {
            return DebugEvent::SoftwareBreakpointHit;
        } else if (r == "HW_BKPT") {
            return DebugEvent::HardwareBreakpointHit;
        } else if (r == "SINGLESTEP") {
            return DebugEvent::Singlestep;
        } else if (r == "HALT") {
            return DebugEvent::ExecutionEnd;
        } else {
            UNREACHABLE;
        }
    }

    void Singlestep() override {
        process->Send("SINGLESTEP");
        CheckResponse("SINGLESTEP error");
    }

    std::map<std::string, int64_t> FetchRegisters() override {
        process->Send("PEEKREGS");
        auto regs = process->Receive();
        auto lines = utils::split_v(*regs, '\n');
        std::map<std::string, int64_t> result;
        for (const auto& line: lines) {
            log_info("Got register '{}'", line);
            auto reg = utils::split(line, ':');
            result[reg.at(0)] = std::stoll(reg.at(1));
        }
        return result;
    }

    void SetRegisters(const std::map<std::string, int64_t>& regs) override {
        for (const auto& [name, val]: regs) {
            if (!IsValidRegisterName(name)) {
                throw DebuggerError(fmt::format("Register name '{}' is not valid!", name));
            }
            log_info("Setting register {} to value {}", name, val);
            process->Send(fmt::format("POKEREGS {} {}", name, val));
            CheckResponse("POKEREGS error");
        }
    }

    size_t TextSize() override {
        process->Send("TEXTSIZE");
        auto response = process->Receive();
        if (!response) {
            throw DebuggerError("TEXTSIZE error");
        }
        size_t result = std::stoull(response->c_str() + strlen("TEXTSIZE:"));
        return result;
    }

    void ResumeExecution() override {
        process->Send("CONTINUE");
        CheckResponse("CONTINUE fail");
    }
    
    void Wait() override {
        auto message = process->Receive();
        if (!message || message != "STOPPED") {
            throw DebuggerError(fmt::format("Expected STOPPED message in Wait()"));
        }
    }
private:
    int64_t GetRegister(std::string_view name) {
        process->Send(fmt::format("PEEKREGS {}", name));
        auto response = process->Receive();
        if (!response) {
            throw DebuggerError("PEEKREGS error");
        }
        auto offset = response->find("VALUE:") + strlen("VALUE:");
        auto str_val = response->substr(offset);
        auto value = std::stoll(response->substr(offset));
        return value;
    }

    bool isGPRegister(std::string_view name) {
        if (name.size() >= 2 
                && name[0] == 'R') {
            auto idx = utils::svtoi64(name.substr(1));
            return 0 <= idx && idx < gen_purpose_regs_count;
        }
        return false;
    }

    bool IsValidRegisterName(std::string_view name) {
        return name == "IP"
               || name == "BP"
               || name == "SP"
               || name == "FLAGS"
               || isGPRegister(name);
    }

    void CheckResponse(std::string_view error_message) {
        auto message = process->Receive();
        if (!message || message != "OK") {
            throw DebuggerError(fmt::format(
                "Error communicating with T86 VM: {}", error_message));
        }
    }

    std::unique_ptr<Messenger> process;
    const size_t data_size{1024};
    const size_t gen_purpose_regs_count{8};
  };
