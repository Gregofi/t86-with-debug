#pragma once

enum class DebugEvent {
    SoftwareBreakpointHit,
    HardwareBreakpointHit,
    Singlestep,
    ExecutionBegin,
    ExecutionEnd,
};

