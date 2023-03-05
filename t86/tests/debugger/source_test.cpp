#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sstream>
#include "debugger/Source/Parser.h"
#include "debugger/Source/LineMapping.h"
#include "debugger/Source/Source.h"
#include "threads_messenger.h"
#include "utils.h"

TEST(SourceLines, Parsing) {
    auto program = R"(
.debug_line
0: 3
1: 3
2: 4
3: 5
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    EXPECT_EQ(info.line_mapping->size(), 4);
    EXPECT_EQ(info.line_mapping->at(0), 3);
    EXPECT_EQ(info.line_mapping->at(1), 3);
    EXPECT_EQ(info.line_mapping->at(2), 4);
    EXPECT_EQ(info.line_mapping->at(3), 5);
}

TEST(SourceLines, ParsingNonContinuous) {
    auto program = R"(
.debug_line
0: 3

5: 3

9:

4
1: 5
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    EXPECT_EQ(info.line_mapping->size(), 4);
    EXPECT_EQ(info.line_mapping->at(0), 3);
    EXPECT_EQ(info.line_mapping->at(5), 3);
    EXPECT_EQ(info.line_mapping->at(9), 4);
    EXPECT_EQ(info.line_mapping->at(1), 5);
}

TEST(SourceLines, ParsingEmpty) {
    auto program = R"(
.debug_line
.text
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    EXPECT_EQ(info.line_mapping->size(), 0);
}

TEST(LineMapping, Basics) {
    auto program = R"(
.debug_line
0: 3
1: 3
2: 4
3: 5
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    ASSERT_TRUE(info.line_mapping);
    LineMapping lm(std::move(*info.line_mapping));
    EXPECT_EQ(lm.GetAddress(0), 3);
    EXPECT_EQ(lm.GetAddress(1), 3);
    EXPECT_EQ(lm.GetAddress(2), 4);
    EXPECT_EQ(lm.GetAddress(3), 5);

    EXPECT_THAT(lm.GetLines(3), testing::ElementsAre(0, 1));
    EXPECT_THAT(lm.GetLines(4), testing::ElementsAre(2));
    EXPECT_THAT(lm.GetLines(5), testing::ElementsAre(3));
}

TEST(Source, LineAndSourceMapping) {
    Source source;
    ASSERT_FALSE(source.AddrToLine(0));

    auto source_code =
R"(int main(void) {
    int i = 5;
    int y = 10;
    return i + y;
}
)";
    auto program = R"(
.debug_line
0: 3
1: 3
2: 4
3: 5
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    ASSERT_TRUE(info.line_mapping);
    LineMapping lm(std::move(*info.line_mapping));
    source.RegisterLineMapping(std::move(lm));
    ASSERT_FALSE(source.AddrToLine(0));
    ASSERT_FALSE(source.AddrToLine(1));
    ASSERT_FALSE(source.AddrToLine(2));
    ASSERT_TRUE(source.AddrToLine(3));
    EXPECT_EQ(source.AddrToLine(3), 1);
    ASSERT_TRUE(source.AddrToLine(4));
    EXPECT_EQ(source.AddrToLine(4), 2);
    ASSERT_TRUE(source.AddrToLine(5));
    EXPECT_EQ(source.AddrToLine(5), 3);
    ASSERT_FALSE(source.AddrToLine(6));

    ASSERT_THAT(source.GetLines(0, 3), testing::IsEmpty());
    SourceFile file(source_code);
    source.RegisterSourceFile(std::move(file));
    auto source_program = source.GetLines(0, 10);
    ASSERT_EQ(source_program.size(), 5);
    EXPECT_EQ(source_program.at(0), "int main(void) {");
    EXPECT_EQ(source_program.at(1), "    int i = 5;");
    EXPECT_EQ(source_program.at(2), "    int y = 10;");
    EXPECT_EQ(source_program.at(3), "    return i + y;");
    EXPECT_EQ(source_program.at(4), "}");
}

    const char* source_code1 =
R"(int main() {
    int a = 5;
    int b = 6;
    return a + b;
})";
    const char* elf1 =
R"(

.text
0 CALL 2
1 HALT
2 PUSH BP
3 MOV BP, SP
4 SUB SP, 2
5 MOV [BP + -1], 5
6 MOV [BP + -2], 6
7 MOV R0, [BP + -1]
8 MOV R1, [BP + -2]
9 ADD R0, R1
10 ADD SP, 2
11 POP BP
12 RET

.debug_line
0: 2
1: 5
2: 6
3: 7
4: 11
)";

TEST(SourceNative, SourceBreakpoints) {
    const size_t REG_COUNT = 6;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
    std::thread t_os(RunCPU, std::move(tm1), elf1, REG_COUNT, 0);
    auto t86 = std::make_unique<T86Process>(std::move(tm2), REG_COUNT, 0);
    Native native(std::move(t86));
    
    std::istringstream iss(elf1);
    dbg::Parser p(iss);
    auto debug_info = p.Parse();
    ASSERT_TRUE(debug_info.line_mapping);
    Source source;
    source.RegisterLineMapping(std::move(*debug_info.line_mapping));
    SourceFile file(source_code1);
    source.RegisterSourceFile(std::move(file));

    for (size_t i = 0; i < 5; ++i) {
        ASSERT_TRUE(source.LineToAddr(i));
    }

    EXPECT_EQ(source.LineToAddr(0), 2);
    EXPECT_EQ(source.LineToAddr(1), 5);
    EXPECT_EQ(source.LineToAddr(2), 6);
    EXPECT_EQ(source.LineToAddr(3), 7);
    EXPECT_EQ(source.LineToAddr(4), 11);

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::ExecutionBegin);
    EXPECT_EQ(source.SetSourceSoftwareBreakpoint(native, 0), 2);
    EXPECT_EQ(source.SetSourceSoftwareBreakpoint(native, 1), 5);
    EXPECT_EQ(source.SetSourceSoftwareBreakpoint(native, 2), 6);
    EXPECT_EQ(source.SetSourceSoftwareBreakpoint(native, 3), 7);
    EXPECT_EQ(source.SetSourceSoftwareBreakpoint(native, 4), 11);
    ASSERT_THROW({source.SetSourceSoftwareBreakpoint(native, 5);}, DebuggerError);

    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    EXPECT_EQ(native.GetIP(), 2);

    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    EXPECT_EQ(native.GetIP(), 5);

    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    EXPECT_EQ(native.GetIP(), 6);

    native.ContinueExecution();
    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    EXPECT_EQ(native.GetIP(), 7);
    // Check BP - 1 and BP - 2
    auto bp = native.GetRegister("BP");
    EXPECT_EQ(native.ReadMemory(bp - 1, 1)[0], 5);
    EXPECT_EQ(native.ReadMemory(bp - 2, 1)[0], 6);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::SoftwareBreakpointHit);
    EXPECT_EQ(native.GetIP(), 11);
    EXPECT_EQ(native.GetRegister("R0"), 11);
    native.ContinueExecution();

    ASSERT_EQ(native.WaitForDebugEvent(), DebugEvent::ExecutionEnd);
    native.Terminate();
    t_os.join();
}
