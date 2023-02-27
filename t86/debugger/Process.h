#pragma once
#include <string_view>
#include <cstdint>
#include <string>
#include <vector>
#include <map>

class Process {
public:
    enum class StopReason {
        SoftwareBreakpoint,
        HardwareBreakpoint,
        Singlestep,
        ExecutionBegin,
        ExecutionEnd,
    };
    
    virtual void WriteText(uint64_t address,
                         const std::vector<std::string> &data) = 0;
    virtual std::vector<std::string> ReadText(uint64_t address,
                                            size_t amount) = 0;

    virtual void WriteMemory(uint64_t address, std::vector<uint64_t> data) = 0;
    virtual std::vector<uint64_t> ReadMemory(uint64_t address, size_t amount) = 0;

    virtual StopReason GetReason() = 0;

    /// Performs singlestep and returns true.
    /// Should throws runtime_error if architecture
    /// does not support singlestepping.
    virtual void Singlestep() = 0;

    virtual std::map<std::string, int64_t> FetchRegisters() = 0;
    virtual void SetRegisters(const std::map<std::string, int64_t>& regs) = 0;
protected:
};

