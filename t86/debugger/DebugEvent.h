#pragma once

#include <cstdint>
#include <variant>

enum class StopReason {
    SoftwareBreakpointHit,
    HardwareBreak,
    Singlestep,
    ExecutionBegin,
    ExecutionEnd,
    CpuError,
};

enum class BPType {
    Software,
    Hardware,
};

enum class WatchpointType {
    Read,
    Write,
};

struct BreakpointHit {
    BPType type;
    uint64_t address;
};

struct WatchpointTrigger {
    WatchpointType type;
    uint64_t address;
};

struct CpuError {
    uint64_t address;
};

struct Singlestep {};
struct ExecutionBegin {};
struct ExecutionEnd {};
using DebugEvent = std::variant<BreakpointHit,
                                WatchpointTrigger,
                                Singlestep,
                                ExecutionBegin,
                                ExecutionEnd,
                                CpuError>;
