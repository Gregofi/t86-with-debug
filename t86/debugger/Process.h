#pragma once
#include <string_view>
#include <cstdint>
#include <string>
#include <vector>
#include <map>

#include "DebugEvent.h"

class Process {
public:
    virtual ~Process() = default;
    virtual void WriteText(uint64_t address,
                         const std::vector<std::string> &data) = 0;
    virtual std::vector<std::string> ReadText(uint64_t address,
                                            size_t amount) = 0;

    virtual void WriteMemory(uint64_t address, std::vector<uint64_t> data) = 0;
    virtual std::vector<uint64_t> ReadMemory(uint64_t address, size_t amount) = 0;

    virtual DebugEvent GetReason() = 0;

    /// Performs singlestep and returns true.
    /// Should throws runtime_error if architecture
    /// does not support singlestepping.
    virtual void Singlestep() = 0;

    virtual std::map<std::string, int64_t> FetchRegisters() = 0;
    virtual std::map<std::string, double> FetchFloatRegisters() = 0;
    virtual void SetRegisters(const std::map<std::string, int64_t>& regs) = 0;
    virtual void SetFloatRegisters(const std::map<std::string, double>& regs) = 0;
    virtual void ResumeExecution() = 0;
    virtual size_t TextSize() = 0;
    virtual void Wait() = 0;
    /// Cause the process to end, the class should not be used
    /// after this function is called.
    virtual void Terminate() = 0;
protected:
};

