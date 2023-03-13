#pragma once

enum class StopReason {
    SoftwareBreakpointHit,
    HardwareBreak,
    Singlestep,
    ExecutionBegin,
    ExecutionEnd,
};

// Is separated into two because there could
// be more debug events, like WatchpointRead,
// HardwareBreakpointHit. 
enum class DebugEvent {
    SoftwareBreakpointHit,
    WatchpointWrite,
    Singlestep,
    ExecutionBegin,
    ExecutionEnd,
};
