#pragma once
#include <istream>
#include "common/threads_messenger.h"
#include "debugger/Process.h"
#include "t86/os.h"

class MockedProcess: public Process {
public:
    MockedProcess(std::vector<std::string> text, std::vector<int64_t> data,
                  std::map<std::string, int64_t> regs): text(std::move(text)),
                                data(std::move(data)), regs(std::move(regs)) { }

    void WriteText(uint64_t address,
                   const std::vector<std::string> &text) override {
        for (size_t i = 0; i < data.size(); ++i) {
            this->text[i + address] = text[i];
        }
    }
    std::vector<std::string> ReadText(uint64_t address,
                                      size_t amount) override {
        std::vector<std::string> result;
        for (size_t i = address; i < address + amount; ++i) {
            result.push_back(text[i]);
        }
        return result;
    }

    void WriteMemory(uint64_t address, const std::vector<int64_t>& data) override {
        for (size_t i = 0; i < data.size(); ++i) {
            this->data[i + address] = data[i];
        }
    }

    std::vector<int64_t> ReadMemory(uint64_t address, size_t amount) override {
        std::vector<int64_t> result;
        for (size_t i = address; i < address + amount; ++i) {
            result.push_back(data[i]);
        }
        return result;
    }

    DebugEvent GetReason() override {
        NOT_IMPLEMENTED;
    }

    /// Performs singlestep and returns true.
    /// Should throws runtime_error if architecture
    /// does not support singlestepping.
    void Singlestep() override {
        NOT_IMPLEMENTED;
    }

    std::map<std::string, int64_t> FetchRegisters() override {
        return regs;
    }
    std::map<std::string, double> FetchFloatRegisters() override {
        NOT_IMPLEMENTED;
    }
    void SetRegisters(const std::map<std::string, int64_t>& regs) override {
        this->regs = regs;
    }
    void SetFloatRegisters(const std::map<std::string, double>& regs) override {
        NOT_IMPLEMENTED;
    }
    void ResumeExecution() override {
        NOT_IMPLEMENTED;
    }
    size_t TextSize() override {
        NOT_IMPLEMENTED;
    }
    void Wait() override {
        NOT_IMPLEMENTED;
    }
    /// Cause the process to end, the class should not be used
    /// after this function is called.
    void Terminate() override {
        NOT_IMPLEMENTED;
    }
protected:
    std::vector<std::string> text;
    std::vector<int64_t> data;
    std::map<std::string, int64_t> regs;
};

inline void RunCPU(std::unique_ptr<ThreadMessenger> messenger,
            const std::string &program, size_t register_cnt = 4, size_t float_cnt = 0) {
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    
    tiny::t86::OS os(register_cnt, float_cnt);
    os.SetDebuggerComms(std::move(messenger));
    os.Run(std::move(p));
}
