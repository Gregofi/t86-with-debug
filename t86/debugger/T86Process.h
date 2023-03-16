#pragma once
#include <string_view>
#include <vector>
#include <memory>
#include <functional>

#include "common/messenger.h"
#include "common/logger.h"
#include "common/helpers.h"
#include "t86-parser/parser.h"
#include "fmt/core.h"
#include "DebuggerError.h"
#include "DebugEvent.h"
#include "Process.h"

class T86Process: public Process {
public:
    T86Process(std::unique_ptr<Messenger> process, size_t gp_reg_cnt = 10,
               size_t float_reg_cnt = 4, size_t data_size = 1024)
                    : process(std::move(process)),
                      data_size(data_size),
                      gen_purpose_regs_count(gp_reg_cnt),
                      float_regs_count(float_reg_cnt) {
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
                   const std::vector<std::string> &data) override;

    /// Returns range of instructions of length 'amount' starting at 'address'.
    /// Caller is responsible that the size of the program is respected.
    /// Use the TextSize for getting the size of text.
    std::vector<std::string> ReadText(uint64_t address, size_t amount);

    void WriteMemory(uint64_t address, const std::vector<int64_t>& data) override;

    std::vector<int64_t> ReadMemory(uint64_t address, size_t amount) override;

    StopReason GetReason() override;

    void Singlestep() override;

    std::map<std::string, int64_t> FetchRegisters() override;

    void SetRegisters(const std::map<std::string, int64_t>& regs) override;

    std::map<std::string, double> FetchFloatRegisters() override;

    void SetFloatRegisters(const std::map<std::string, double>& regs) override;

    std::map<std::string, uint64_t> FetchDebugRegisters() override;

    void SetDebugRegisters(const std::map<std::string, uint64_t>& regs) override;

    size_t TextSize() override;

    void ResumeExecution() override;

    void Wait() override;

    void Terminate() override;
private:
    template<typename T>
    std::map<std::string, T> FetchRegistersOfType() {
        auto regs = process->Receive();
        auto lines = utils::split_v(*regs, '\n');
        std::map<std::string, T> result;
        for (const auto& line: lines) {
            log_info("Got register '{}'", line);
            auto reg = utils::split(line, ':');
            result[reg.at(0)] = *utils::svtonum<T>(reg.at(1));
        }
        return result;
    }

    int64_t GetRegister(std::string_view name);

    bool isGPRegister(std::string_view name) const;

    bool IsValidRegisterName(std::string_view name) const;

    bool IsValidFloatRegisterName(std::string_view name) const;

    bool IsValidDebugRegisterName(std::string_view name) const;

    void CheckResponse(std::string_view error_message);

    std::unique_ptr<Messenger> process;
    const size_t data_size{1024};
    const size_t gen_purpose_regs_count{8};
    const size_t float_regs_count{4};
    const size_t debug_register_cnt{5};
  };
