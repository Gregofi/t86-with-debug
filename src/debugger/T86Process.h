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

    /// Write into the memory at 'address' the contents of 'data'.
    /// If the write is out of bounds then an exception is thrown
    /// and not data is written.
    void WriteMemory(uint64_t address, const std::vector<int64_t>& data) override;

    /// Read an 'amount' of data from memory at 'address'.
    /// Exception is throw on out of bounds read.
    std::vector<int64_t> ReadMemory(uint64_t address, size_t amount) override;

    /// Returns the reason why program stopped.
    StopReason GetReason() override;

    /// Performs HW level singlestep.
    void Singlestep() override;

    /// Returns map of all normal registers
    std::map<std::string, int64_t> FetchRegisters() override;

    /// Sets normal registers.
    /// A FetchRegisters should be used first. Modify the registers in the
    /// returned map and pass that map to this function.
    void SetRegisters(const std::map<std::string, int64_t>& regs) override;

    /// Returns float registers.
    std::map<std::string, double> FetchFloatRegisters() override;

    /// Sets float registers.
    /// The usage should be the same as with 'SetRegisters'.
    void SetFloatRegisters(const std::map<std::string, double>& regs) override;

    /// Returns debug registers.
    std::map<std::string, uint64_t> FetchDebugRegisters() override;

    /// Sets debug registers.
    /// The usage should be the same as with 'SetRegisters'.
    void SetDebugRegisters(const std::map<std::string, uint64_t>& regs) override;

    /// Returns the size of the text section.
    size_t TextSize() override;

    /// Continues execution.
    /// After this function is called, no other methods from this class
    /// can be invoked but the 'Wait' method.
    void ResumeExecution() override;

    /// Waits until the VM stops, must be called
    /// after previous call to ResumeExecution.
    void Wait() override;

    /// Terminates the process. Any subsequent call to any other
    /// method is undefined after this.
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
