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
    ASSERT_EQ(native.GetRegister("IP"), 0);
    ASSERT_EQ(native.GetRegister("R0"), 0);
    native.PerformSingleStep();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::Singlestep);
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
