#pragma once

namespace tiny::t86 {
class Cpu;

/// A debugging interface provided by the VM.
class Debug {
public:
    Debug(int port, Cpu& cpu): cpu(cpu) {}

    enum class BreakReason {
        SoftwareBreakpoint,
        HardwareBreakpoint,
        SingleStep,
        Halt,
    };

    /// Opens connection at specified port and
    /// waits for debugger to connect.
    /// This call is blocking.
    bool OpenConnection();

    /// Use to pass control to the debug interface
    /// which will communicate with the client.
    /// Should be called on any break situation.
    bool Work(BreakReason reason);
private:
    Cpu& cpu;

};
}
