#include "t86/os.h"
#include <thread>
#include "bench_lib.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include "common/threads_messenger.h"
#include "debugger/Process.h"
#include "debugger/Source/Parser.h"
#include "t86/os.h"
#include "debugger/Source/Source.h"

using namespace tiny::t86;

inline void RunCPU(std::unique_ptr<ThreadMessenger> messenger,
            const std::string& path) {
    std::ifstream ifs{TEST_CASE_DIRECTORY + path};
    Parser parser(ifs);
    auto p = parser.Parse();
    
    tiny::t86::OS os;
    os.SetDebuggerComms(std::move(messenger));
    os.Run(std::move(p));
}

class NativeRunner: public bench::Fixture {
public:
    void Run(const std::string& path) {
        auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
        auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
        t_os = std::thread(RunCPU, std::move(tm1), path);
        auto t86 = std::make_unique<T86Process>(std::move(tm2));
        native = std::make_optional<Native>(std::move(t86));
    }

    void Teardown() override {
        native->Terminate();
        t_os.join();
    }

    void BenchContinue() {
        native->WaitForDebugEvent();
        native->ContinueExecution();
        native->WaitForDebugEvent();
    }

    void BenchBP(int location) {
        native->WaitForDebugEvent();
        native->SetBreakpoint(location);
        native->ContinueExecution();
        int bp_hits = 0;
        while (std::holds_alternative<BreakpointHit>(native->WaitForDebugEvent())) {
            bp_hits += 1;
            native->ContinueExecution();
        }
        fmt::print("BP hits: {}\n", bp_hits);
    }

    void BenchBPMemoryRegs(int location) {
        native->WaitForDebugEvent();
        native->SetBreakpoint(location);
        native->ContinueExecution();
        while (std::holds_alternative<BreakpointHit>(native->WaitForDebugEvent())) {
            native->GetIP();
            native->ReadMemory(0, 100);
            native->ContinueExecution();
        }
    }

    void BenchStepOver(int call_location) {
        native->WaitForDebugEvent();

        native->SetBreakpoint(call_location);
        native->ContinueExecution();
        native->WaitForDebugEvent();

        native->PerformStepOver();
        native->ContinueExecution();

        native->WaitForDebugEvent();
    }

    void BenchStepOut(int location) {
        native->WaitForDebugEvent();

        native->SetBreakpoint(location);
        native->ContinueExecution();
        native->WaitForDebugEvent();
        native->DisableSoftwareBreakpoint(location);

        native->PerformStepOut();
        native->ContinueExecution();

        native->WaitForDebugEvent();
    }

    std::optional<Native> native;
    std::thread t_os;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
};

BENCH(NativeRunner, QuicksortContinue) {
    Run("/benches/quicksort.t86");
    BenchContinue();
}

BENCH(NativeRunner, PrimesContinue) {
    Run("/benches/prime.t86");
    BenchContinue();
}

BENCH(NativeRunner, QuicksortBP) {
    Run("/benches/quicksort.t86");
    BenchBP(133);
}

BENCH(NativeRunner, PrimesBP) {
    Run("/benches/prime.t86");
    BenchBP(17);
}

BENCH(NativeRunner, QuicksortBPMemoryRegister) {
    Run("/benches/quicksort.t86");
    BenchBPMemoryRegs(133);
}

BENCH(NativeRunner, PrimesBPMemoryRegister) {
    Run("/benches/prime.t86");
    BenchBPMemoryRegs(17);
}

BENCH(NativeRunner, QuicksortStepOver) {
    Run("/benches/quicksort.t86");
    BenchStepOver(207);
}

BENCH(NativeRunner, PrimesStepOver) {
    Run("/benches/prime.t86");
    BenchStepOver(43);
}

BENCH(NativeRunner, QuicksortStepOut) {
    Run("/benches/quicksort.t86");
    BenchStepOut(131);
}

BENCH(NativeRunner, PrimesStepOut) {
    Run("/benches/prime.t86");
    BenchStepOut(4);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fmt::print("usage: {} bench\n", argv[0]);
        return 1;
    }
    RUN_BENCHMARK(QuicksortContinue);
    RUN_BENCHMARK(PrimesContinue);
    RUN_BENCHMARK(QuicksortBP);
    RUN_BENCHMARK(PrimesBP);
    RUN_BENCHMARK(QuicksortBPMemoryRegister);
    RUN_BENCHMARK(PrimesBPMemoryRegister);
    RUN_BENCHMARK(QuicksortStepOver);
    RUN_BENCHMARK(PrimesStepOver);
    RUN_BENCHMARK(QuicksortStepOut);
    RUN_BENCHMARK(PrimesStepOut);
    fmt::print("Unknown benchmark\n");
}
