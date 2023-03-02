#include <gtest/gtest.h>
#include "debugger/Native.h"
#include "common/threads_messenger.h"
#include "t86/cpu.h"
#include "utils.h"

TEST(NativeWT86Test, Basics) {
    const size_t REG_COUNT = 3;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
    auto program = R"(
.text

0 MOV R0, 1
1 MOV R1, 2
2 ADD R0, R1
3 MOV R2, R0
4 HALT
)";
    std::thread t_os(RunCPU, std::move(tm1), program, REG_COUNT);
    auto t86 = std::make_unique<T86Process>(std::move(tm2), REG_COUNT);
    Native native(std::move(t86));

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::ExecutionBegin); 
    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::ExecutionEnd);
    native.ContinueExecution();
    t_os.join();
}

TEST(NativeWT86Test, Reading) {
    const size_t REG_COUNT = 3;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
    auto program = R"(
.text

0 MOV R0, 3
1 MOV R1, 2
2 ADD R0, R1
3 MOV R2, R0
4 HALT
)";
    std::thread t_os(RunCPU, std::move(tm1), program, REG_COUNT);
    auto t86 = std::make_unique<T86Process>(std::move(tm2), REG_COUNT);
    Native native(std::move(t86));
    native.WaitForDebugEvent();

    auto text = native.ReadText(0, 5);
    ASSERT_EQ(text.size(), 5);
    auto it = text.begin();
    ASSERT_EQ(*it++, "MOV R0, 3");
    ASSERT_EQ(*it++, "MOV R1, 2");
    ASSERT_EQ(*it++, "ADD R0, R1");
    ASSERT_EQ(*it++, "MOV R2, R0");
    ASSERT_EQ(*it++, "HALT");

    ASSERT_THROW({
        native.ReadText(0, 6);
    }, DebuggerError);
    ASSERT_THROW({
        native.ReadText(5, 1);
    }, DebuggerError);
    ASSERT_NO_THROW({
        native.ReadText(4, 1);
    });

    ASSERT_EQ(native.GetRegister("IP"), 0);
    ASSERT_EQ(native.GetRegister("R0"), 0);
    ASSERT_EQ(native.PerformSingleStep(), DebugEvent::Singlestep);
    ASSERT_EQ(native.GetRegister("IP"), 1);
    ASSERT_EQ(native.GetRegister("R0"), 3);
    
    ASSERT_THROW({
        native.GetRegister("R3"); 
    }, DebuggerError);

    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::ExecutionEnd);
    native.ContinueExecution();
    t_os.join();
}

TEST(NativeWT86Test, Writing) {
    const size_t REG_COUNT = 3;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
    auto program = R"(
.text

0 MOV R0, 3
1 MOV R1, 2
2 ADD R0, R1
3 MOV R2, R0
4 HALT
)";
    std::thread t_os(RunCPU, std::move(tm1), program, REG_COUNT);
    auto t86 = std::make_unique<T86Process>(std::move(tm2), REG_COUNT);
    Native native(std::move(t86));
    native.WaitForDebugEvent();
    
    native.WriteText(0, {"MOV R2, 1", "MOV R1, 3"});
    auto text = native.ReadText(0, 2);
    ASSERT_EQ(text.size(), 2);
    ASSERT_EQ(text[0], "MOV R2, 1");
    ASSERT_EQ(text[1], "MOV R1, 3");
    ASSERT_THROW({
        native.WriteText(4, {"HALT", "HALT"});
    }, DebuggerError);
    ASSERT_NO_THROW({
        native.WriteText(4, {"HALT"});
    });
    
    native.SetRegister("R0", 1);
    ASSERT_THROW({
        native.SetRegister("R3", 2);
    }, DebuggerError);
    ASSERT_EQ(native.GetRegister("R0"), 1);

    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::ExecutionEnd);
    ASSERT_EQ(native.GetRegister("R0"), 4);
    native.ContinueExecution();
    t_os.join();
}

TEST(NativeWT86Test, SimpleBreakpoint) {
    const size_t REG_COUNT = 3;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
    auto program = R"(
.text

0 MOV R0, 3
1 MOV R1, 2
2 ADD R0, R1
3 MOV R2, R0
4 HALT
)";
    std::thread t_os(RunCPU, std::move(tm1), program, REG_COUNT);
    auto t86 = std::make_unique<T86Process>(std::move(tm2), REG_COUNT);
    Native native(std::move(t86));
    native.WaitForDebugEvent();
    native.SetBreakpoint(2);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.GetRegister("IP"), 2);
    ASSERT_EQ(native.GetRegister("R0"), 3);
    ASSERT_EQ(native.GetRegister("R1"), 2);

    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::ExecutionEnd);
    // Check that the ADD R0, R1 was executed even though it
    // was replaced by a breakpoint.
    ASSERT_EQ(native.GetRegister("R2"), 5);
    native.ContinueExecution();
    t_os.join();
}

TEST(NativeWT86Test, StepOverBreakpoint) {
    const size_t REG_COUNT = 3;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
    auto program = R"(
.text

0 MOV R0, 3
1 MOV R1, 2
2 ADD R0, R1
3 MOV R2, R0
4 HALT
)";
    std::thread t_os(RunCPU, std::move(tm1), program, REG_COUNT);
    auto t86 = std::make_unique<T86Process>(std::move(tm2), REG_COUNT);
    Native native(std::move(t86));
    native.WaitForDebugEvent();
    native.SetBreakpoint(2);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.GetRegister("R0"), 3);
    ASSERT_EQ(native.GetRegister("R1"), 2);

    ASSERT_EQ(native.PerformSingleStep(), DebugEvent::Singlestep);
    // Check that the ADD R0, R1 was executed even though it
    // was replaced by a breakpoint.
    ASSERT_EQ(native.GetRegister("R0"), 5);
    ASSERT_EQ(native.GetRegister("R2"), 0);
    native.ContinueExecution();
    native.WaitForDebugEvent();
    ASSERT_EQ(native.GetRegister("R0"), 5);
    ASSERT_EQ(native.GetRegister("R2"), 5);
    native.ContinueExecution();
    t_os.join();
}

TEST(NativeWT86Test, BreakpointAtHaltSinglestep) {
    const size_t REG_COUNT = 3;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
    auto program = R"(
.text

0 MOV R0, 3
1 MOV R1, 2
2 ADD R0, R1
3 MOV R2, R0
4 HALT
)";
    std::thread t_os(RunCPU, std::move(tm1), program, REG_COUNT);
    auto t86 = std::make_unique<T86Process>(std::move(tm2), REG_COUNT);
    Native native(std::move(t86));
    native.WaitForDebugEvent();
    native.SetBreakpoint(4);

    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.PerformSingleStep(), DebugEvent::ExecutionEnd);
    native.ContinueExecution();
    t_os.join();
}

TEST(NativeWT86Test, BreakpointAtHaltContinue) {
    const size_t REG_COUNT = 3;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
    auto program = R"(
.text

0 MOV R0, 3
1 MOV R1, 2
2 ADD R0, R1
3 MOV R2, R0
4 HALT
)";
    std::thread t_os(RunCPU, std::move(tm1), program, REG_COUNT);
    auto t86 = std::make_unique<T86Process>(std::move(tm2), REG_COUNT);
    Native native(std::move(t86));
    native.WaitForDebugEvent();
    native.SetBreakpoint(4);

    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::ExecutionEnd);
    native.ContinueExecution();
    t_os.join();
}

TEST(NativeWT86Test, MultipleBreakpointHits) {
    const size_t REG_COUNT = 3;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
    auto program = R"(
.text

0 NOP
1 MOV BP, SP
2 SUB SP, 1
3 MOV [BP + -1], 0
4 JMP 14

# Check if iter variable is odd and print if so
5 MOV R0, [BP + -1]
6 AND R0, 1
7 CMP R0, 1
8 JNE 11

9 MOV R0, [BP + -1]
10 ADD R1, R0

# Increment iteration variable
11 MOV R0, [BP + -1]
12 ADD R0, 1
13 MOV [BP + -1], R0

# Condition check
14 MOV R0, [BP + -1]
15 CMP R0, 9
16 JLE 5

17 HALT
)";
    std::thread t_os(RunCPU, std::move(tm1), program, REG_COUNT);
    auto t86 = std::make_unique<T86Process>(std::move(tm2), REG_COUNT);
    Native native(std::move(t86));
    native.WaitForDebugEvent();
    native.SetBreakpoint(10);
    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.GetRegister("R0"), 1);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.GetRegister("R0"), 3);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.GetRegister("R0"), 5);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.GetRegister("R0"), 7);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.GetRegister("R0"), 9);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::ExecutionEnd);
    native.ContinueExecution();
    t_os.join();
}

TEST(NativeWT86Test, EnableDisableBreakpoints) {
    const size_t REG_COUNT = 3;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
    auto program = R"(
.text

0 NOP
1 MOV BP, SP
2 SUB SP, 1
3 MOV [BP + -1], 0
4 JMP 14

# Check if iter variable is odd and print if so
5 MOV R0, [BP + -1]
6 AND R0, 1
7 CMP R0, 1
8 JNE 11

9 MOV R0, [BP + -1]
10 ADD R1, R0

# Increment iteration variable
11 MOV R0, [BP + -1]
12 ADD R0, 1
13 MOV [BP + -1], R0

# Condition check
14 MOV R0, [BP + -1]
15 CMP R0, 9
16 JLE 5

17 HALT
)";
    std::thread t_os(RunCPU, std::move(tm1), program, REG_COUNT);
    auto t86 = std::make_unique<T86Process>(std::move(tm2), REG_COUNT);
    Native native(std::move(t86));
    native.WaitForDebugEvent();
    native.SetBreakpoint(10);
    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.GetRegister("R0"), 1);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.GetRegister("R0"), 3);
    native.SetBreakpoint(13);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.GetRegister("IP"), 13);
    native.DisableSoftwareBreakpoint(13);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.GetRegister("R0"), 5);
    ASSERT_EQ(native.GetRegister("IP"), 10);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.GetRegister("R0"), 7);
    ASSERT_EQ(native.GetRegister("IP"), 10);
    native.DisableSoftwareBreakpoint(10);
    native.EnableSoftwareBreakpoint(13);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.GetRegister("IP"), 13);
    native.DisableSoftwareBreakpoint(13);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::ExecutionEnd);
    ASSERT_EQ(native.GetRegister("R1"), 25);
    native.ContinueExecution();
    t_os.join();
}

TEST(NativeWT86Test, PeekTextWithBreakpoints) {
    const size_t REG_COUNT = 3;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
    auto program = R"(
.text

0 MOV R0, 3
1 MOV R1, 2
2 ADD R0, R1
3 MOV R2, R0
4 HALT
)";
    std::thread t_os(RunCPU, std::move(tm1), program, REG_COUNT);
    auto t86 = std::make_unique<T86Process>(std::move(tm2), REG_COUNT);
    Native native(std::move(t86));
    native.WaitForDebugEvent();
    native.SetBreakpoint(1);
    auto text = native.ReadText(0, 3);
    ASSERT_EQ(text[0], "MOV R0, 3");
    ASSERT_EQ(text[1], "MOV R1, 2"); // Not BKPT!!
    ASSERT_EQ(text[2], "ADD R0, R1");

    native.DisableSoftwareBreakpoint(1);
    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::ExecutionEnd);
    native.ContinueExecution();
    t_os.join();
}

TEST(NativeWT86Test, PokeTextWithBreakpoints) {
    const size_t REG_COUNT = 3;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
    auto program = R"(
.text

0 MOV R0, 3
1 MOV R1, 2
2 ADD R0, R1
3 MOV R2, R0
4 HALT
)";
    std::thread t_os(RunCPU, std::move(tm1), program, REG_COUNT);
    auto t86 = std::make_unique<T86Process>(std::move(tm2), REG_COUNT);
    Native native(std::move(t86));
    native.WaitForDebugEvent();
    native.SetBreakpoint(1);
    native.WriteText(0, {
        "MOV R0, 3",
        "MOV R1, 2",
        "ADD R0, R1",
    });
    auto text = native.ReadText(0, 3);
    ASSERT_EQ(text[0], "MOV R0, 3");
    ASSERT_EQ(text[1], "MOV R1, 2");
    ASSERT_EQ(text[2], "ADD R0, R1");

    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    ASSERT_EQ(native.GetRegister("R0"), 3);
    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::ExecutionEnd);
    ASSERT_EQ(native.GetRegister("R1"), 2);
    native.ContinueExecution();
    t_os.join();
}

TEST(NativeWT86Test, BreakpointsInvalid) {
    const size_t REG_COUNT = 3;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
    auto program = R"(
.text

0 MOV R0, 3
1 MOV R1, 2
2 ADD R0, R1
3 MOV R2, R0
4 HALT
)";
    std::thread t_os(RunCPU, std::move(tm1), program, REG_COUNT);
    auto t86 = std::make_unique<T86Process>(std::move(tm2), REG_COUNT);
    Native native(std::move(t86));
    native.WaitForDebugEvent();

    ASSERT_THROW({native.EnableSoftwareBreakpoint(2);}, DebuggerError);
    ASSERT_THROW({native.DisableSoftwareBreakpoint(2);}, DebuggerError);
    ASSERT_THROW({native.UnsetBreakpoint(2);}, DebuggerError);
    native.SetBreakpoint(2);
    ASSERT_THROW({native.SetBreakpoint(2);}, DebuggerError);
    native.UnsetBreakpoint(2);
    ASSERT_THROW({native.UnsetBreakpoint(2);}, DebuggerError);
    ASSERT_THROW({native.EnableSoftwareBreakpoint(2);}, DebuggerError);
    ASSERT_THROW({native.DisableSoftwareBreakpoint(2);}, DebuggerError);
    
    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::ExecutionEnd);
    ASSERT_EQ(native.GetRegister("R0"), 5);
    ASSERT_EQ(native.GetRegister("R1"), 2);
    ASSERT_EQ(native.GetRegister("R2"), 5);
    native.ContinueExecution();
    t_os.join();
}
