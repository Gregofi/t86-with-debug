#pragma once
#include <string_view>
#include <vector>
#include <memory>
#include <functional>

#include "common/messenger.h"
#include "common/helpers.h"
#include "fmt/core.h"
#include "DebuggerError.h"
#include "Process.h"

class T86Process: public Process {
public:
  T86Process(std::unique_ptr<Messenger> process)
      : process(std::move(process)) {}

    void WriteText(uint64_t address,
                   const std::vector<std::string> &data) override {
        for (size_t i = 0; i < data.size(); ++i) {
            process->Send(fmt::format("POKETEXT {} {}", address + i, data[i]));
            CheckResponse("POKETEXT error");
        }
    }

    std::vector<std::string> ReadText(uint64_t address, size_t amount) override {
        std::vector<std::string> result;
        for (size_t i = 0; i < amount; ++i) {
            process->Send(fmt::format("PEEKTEXT {}", i + address));
            auto text = process->Receive();
            if (!text) {
                throw DebuggerError("Peektext error");
            }
            size_t offset = text->find_last_of(':');
            result.emplace_back(text->substr(offset + 1));
        }
        return result;
    }

    void WriteMemory(uint64_t address, std::vector<uint64_t> data) override {
        for (size_t i = 0; i < data.size(); ++i) {
            process->Send(fmt::format("POKEDATA {} {}", address + i, data[i]));
            CheckResponse("POKEDATA error");
        }
    }

    std::vector<uint64_t> ReadMemory(uint64_t address, size_t amount) override {
        std::vector<uint64_t> result;
        for (size_t i = 0; i < amount; ++i) {
            process->Send(fmt::format("PEEKDATA {}", i + address));
            auto message = process->Receive();
            if (!message) {
                throw DebuggerError(fmt::format("PEEKDATA error"));
            }
            
            size_t val_offset = message->find("VALUE:");
            auto num_s = message->substr(val_offset + strlen("VALUE:"));
            result.emplace_back(std::stoll(num_s));
        }
        return result;
    }

    StopReason GetReason() override {
        process->Send("REASON");
        auto reason_opt = process->Receive();
        if (!reason_opt) {
            throw DebuggerError("REASON error");
        }
        auto &r = *reason_opt;
        if (r == "START") {
            return StopReason::ExecutionBegin;
        } else if (r == "SW_BKPT") {
            return StopReason::SoftwareBreakpoint;
        } else if (r == "HW_BKPT") {
            return StopReason::HardwareBreakpoint;
        } else if (r == "SINGLESTEP") {
            return StopReason::Singlestep;
        } else if (r == "HALT") {
            return StopReason::ExecutionEnd;
        } else {
            UNREACHABLE;
        }
    }

    void Singlestep() override {
        process->Send("SINGLESTEP");
        CheckResponse("SINGLESTEP error");
    }

    std::map<std::string, int64_t> FetchRegisters() override {

    }

    void SetRegisters(const std::map<std::string, int64_t>& regs) override {
    }
private:
    void CheckResponse(std::string_view error_message) {
        auto message = process->Receive();
        if (!message || message != "OK") {
            throw DebuggerError(fmt::format(
                "Error communicating with T86 VM: {}", error_message));
        }
    }

    std::unique_ptr<Messenger> process;
};

