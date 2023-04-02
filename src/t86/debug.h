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
#include "t86-parser/parser.h"

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
        CpuError,
    };

    std::string ReasonToString(BreakReason reason);

    size_t svtoidx(std::string_view s);

    Register TranslateToRegister(std::string_view s);

    FloatRegister TranslateToFloatRegister(std::string_view s);

    size_t TranslateToDebugRegister(std::string_view s);

    std::unique_ptr<Instruction> ParseInstruction(std::string_view s);

    std::string FloatRegistersToString() const;

    std::string RegistersToString() const;

    std::string DebugRegistersToString() const;

    /// Use to pass control to the debug interface
    /// which will communicate with the client.
    /// Should be called on any break situation.
    bool Work(BreakReason reason);
private:
    Cpu& cpu;
    std::unique_ptr<Messenger> messenger;
};
}
