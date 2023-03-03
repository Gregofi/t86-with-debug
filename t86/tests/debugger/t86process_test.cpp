#include <gtest/gtest.h>
#include "debugger/T86Process.h"
#include "t86-parser/parser.h"
#include "t86/os.h"
#include "common/threads_messenger.h"
#include "utils.h"

class HardcodedMessenger : public Messenger {
public:
    HardcodedMessenger(std::queue<std::string>& in, std::vector<std::string>& out): in(in), out(out) {}
    std::queue<std::string>& in;
    std::vector<std::string>& out;
    void Send(const std::string& s) override {
        out.push_back(s);
    }
    
    std::optional<std::string> Receive() override {
        if (in.empty()) {
            return std::nullopt;
        }
        auto x = in.front();
        in.pop();
        return x;
    }
};

TEST(T86ProcessTestIsolated, ReadRegisters) {
    std::queue<std::string> in({
        "IP:0\n"
        "BP:1\n"
        "SP:2\n"
        "FLAGS:33\n"
        "R0:3\n"
        "R1:-12\n"
    });
    std::vector<std::string> out;
    T86Process process(std::make_unique<HardcodedMessenger>(in, out), 2);

    auto regs = process.FetchRegisters();
    ASSERT_EQ(regs.size(), 6);
    ASSERT_EQ(regs["IP"], 0);
    ASSERT_EQ(regs["BP"], 1);
    ASSERT_EQ(regs["SP"], 2);
    ASSERT_EQ(regs["FLAGS"], 33);
    ASSERT_EQ(regs["R0"], 3);
    ASSERT_EQ(regs["R1"], -12);
}

TEST(T86ProcessTestIsolated, WriteRegisters) {
    std::queue<std::string> in({
            "OK",
            "OK",
            "OK",
            "OK",
            "OK",
            "OK",
    });
    std::vector<std::string> out;
    T86Process process(std::make_unique<HardcodedMessenger>(in, out), 2);

    std::map<std::string, int64_t> regs = {
        {"IP", 1},
        {"BP", 2},
        {"SP", 3},
        {"FLAGS", 4},
        {"R0", 5},
        {"R1", 6},
    };
    process.SetRegisters(regs);
    std::set<std::string> res(out.begin(), out.end());
    ASSERT_EQ(res.size(), 6);
    ASSERT_TRUE(res.count("POKEREGS IP 1"));
    ASSERT_TRUE(res.count("POKEREGS BP 2"));
    ASSERT_TRUE(res.count("POKEREGS SP 3"));
    ASSERT_TRUE(res.count("POKEREGS FLAGS 4"));
    ASSERT_TRUE(res.count("POKEREGS R0 5"));
    ASSERT_TRUE(res.count("POKEREGS R1 6"));

    out.clear();

    in = std::queue<std::string>({"OK", "OK"});
    regs = {
        {"IP", 1},
        {"R0", 5},
    };
    process.SetRegisters(regs);
    res = std::set<std::string>(out.begin(), out.end());
    ASSERT_EQ(res.size(), 2);
    ASSERT_TRUE(res.count("POKEREGS IP 1"));
    ASSERT_TRUE(res.count("POKEREGS R0 5"));
}

TEST(T86ProcessTest, WrongRegisters) {
    std::queue<std::string> in({
            "OK",
            "OK",
            "OK",
            "OK",
            "OK",
            "OK",
    });
    std::vector<std::string> out;
    T86Process process(std::make_unique<HardcodedMessenger>(in, out), 2);

    std::map<std::string, int64_t> regs = {
        {"IP", 1},
        {"BP", 2},
        {"NonExistingName", 3},
        {"FLAGS", 4},
        {"R0", 5},
        {"R1", 6},
    };
    EXPECT_THROW({
        process.SetRegisters(regs);
    }, DebuggerError);
    regs = {
        {"IP", 1},
        {"BP", 2},
        {"R3", 3}, // Only two GP registers are in T86Process
        {"FLAGS", 4},
        {"R0", 5},
        {"R1", 6},
    };
    EXPECT_THROW({
        process.SetRegisters(regs);
    }, DebuggerError);
}

TEST(CommunicationTest, Communication) {
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    ThreadMessenger tm2(q2, q1);
    auto program = R"(
.text

0 MOV R0, 1
1 MOV R1, 2
2 ADD R0, R1
3 MOV R2, R0
4 HALT
)";
    std::thread t_os(RunCPU, std::move(tm1), program, 3);
    ASSERT_EQ(*tm2.Receive(), "STOPPED");
    tm2.Send("CONTINUE");
    ASSERT_EQ(*tm2.Receive(), "OK");
    ASSERT_EQ(*tm2.Receive(), "STOPPED");
    tm2.Send("CONTINUE");
    t_os.join();
}

TEST(T86ProcessCpuTest, StopReason) {
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

    auto t86 = T86Process(std::move(tm2), REG_COUNT);
    t86.Wait(); 
    ASSERT_EQ(DebugEvent::ExecutionBegin, t86.GetReason());
    t86.ResumeExecution();
    t86.Wait(); 
    ASSERT_EQ(DebugEvent::ExecutionEnd, t86.GetReason());
    t86.ResumeExecution();

    t_os.join();
}

TEST(T86ProcessCpuTest, PeekRegisters) {
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

    auto t86 = T86Process(std::move(tm2), REG_COUNT);
    t86.Wait(); 
    ASSERT_EQ(DebugEvent::ExecutionBegin, t86.GetReason());
    auto regs = t86.FetchRegisters();
    ASSERT_EQ(regs.at("IP"), 0);
    ASSERT_EQ(regs.at("BP"), 1024);
    ASSERT_EQ(regs.at("SP"), 1024);
    ASSERT_EQ(regs.at("R0"), 0);
    ASSERT_EQ(regs.at("R1"), 0);
    ASSERT_EQ(regs.at("R2"), 0);
    t86.ResumeExecution();
    t86.Wait(); 
    regs = t86.FetchRegisters();
    ASSERT_EQ(regs.at("IP"), 5);
    ASSERT_EQ(regs.at("BP"), 1024);
    ASSERT_EQ(regs.at("SP"), 1024);
    ASSERT_EQ(regs.at("R0"), 3);
    ASSERT_EQ(regs.at("R1"), 2);
    ASSERT_EQ(regs.at("R2"), 3);
    ASSERT_EQ(DebugEvent::ExecutionEnd, t86.GetReason());
    t86.ResumeExecution();

    t_os.join();
}

TEST(T86ProcessCpuTest, SingleSteps) {
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

    auto t86 = T86Process(std::move(tm2), REG_COUNT);
    t86.Wait(); 
    auto regs = t86.FetchRegisters();
    ASSERT_EQ(regs.at("IP"), 0);
    ASSERT_EQ(regs.at("R0"), 0);
    ASSERT_EQ(regs.at("R1"), 0);
    ASSERT_EQ(regs.at("R2"), 0);
    t86.Singlestep();
    t86.Wait();
    ASSERT_EQ(t86.GetReason(), DebugEvent::Singlestep);
    regs = t86.FetchRegisters();
    ASSERT_EQ(regs.at("IP"), 1);
    ASSERT_EQ(regs.at("R0"), 1);
    ASSERT_EQ(regs.at("R1"), 0);
    ASSERT_EQ(regs.at("R2"), 0);
    t86.Singlestep();
    t86.Wait();
    regs = t86.FetchRegisters();
    ASSERT_EQ(regs.at("IP"), 2);
    ASSERT_EQ(regs.at("R0"), 1);
    ASSERT_EQ(regs.at("R1"), 2);
    ASSERT_EQ(regs.at("R2"), 0);
    t86.Singlestep();
    t86.Wait();
    regs = t86.FetchRegisters();
    ASSERT_EQ(regs.at("IP"), 3);
    ASSERT_EQ(regs.at("R0"), 3);
    ASSERT_EQ(regs.at("R1"), 2);
    ASSERT_EQ(regs.at("R2"), 0);
    t86.Singlestep();
    t86.Wait();
    regs = t86.FetchRegisters();
    ASSERT_EQ(regs.at("IP"), 4);
    ASSERT_EQ(regs.at("R0"), 3);
    ASSERT_EQ(regs.at("R1"), 2);
    ASSERT_EQ(regs.at("R2"), 3);
    t86.Singlestep();
    t86.Wait();
    regs = t86.FetchRegisters();
    ASSERT_EQ(regs.at("IP"), 5);
    ASSERT_EQ(regs.at("R0"), 3);
    ASSERT_EQ(regs.at("R1"), 2);
    ASSERT_EQ(regs.at("R2"), 3);
    ASSERT_EQ(t86.GetReason(), DebugEvent::ExecutionEnd);
    t86.ResumeExecution();
    t_os.join();
}

TEST(T86ProcessCpuTest, SetRegisters) {
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

    auto t86 = T86Process(std::move(tm2), REG_COUNT);
    t86.Wait(); 
    auto regs1 = t86.FetchRegisters();
    t86.SetRegisters(regs1);
    auto regs2 = t86.FetchRegisters();
    ASSERT_EQ(regs1, regs2);
    
    regs1.at("R0") = 5;
    t86.SetRegisters(regs1);
    regs2 = t86.FetchRegisters();
    ASSERT_EQ(regs2.at("R0"), 5);

    // Ex: MOV R0, 1
    t86.Singlestep();
    t86.Wait();
    // Ex: MOV R1, 2
    t86.Singlestep();
    t86.Wait();

    regs1 = t86.FetchRegisters();
    regs1.at("R0") = 5;
    t86.SetRegisters(regs1);
    // Ex: ADD R0, R1 but R0 = 5
    t86.Singlestep();
    t86.Wait();
    regs2 = t86.FetchRegisters();

    ASSERT_EQ(regs2.at("R0"), 7);

    // Set IP
    regs2.at("IP") = 2;
    t86.SetRegisters(regs2);
    t86.Singlestep();
    t86.Wait();
    regs2 = t86.FetchRegisters();
    
    ASSERT_EQ(regs2.at("IP"), 3);
    ASSERT_EQ(regs2.at("R0"), 9);
    t86.ResumeExecution();
    t86.Wait();
    ASSERT_EQ(t86.GetReason(), DebugEvent::ExecutionEnd);
    t86.ResumeExecution();
    t_os.join();
}

TEST(T86ProcessCpuTest, PeekText) {
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

    auto t86 = T86Process(std::move(tm2), REG_COUNT);
    t86.Wait();
    auto v = t86.ReadText(0, 1);
    ASSERT_EQ(v.size(), 1);
    ASSERT_EQ(v[0], "MOV R0, 1");

    v = t86.ReadText(2, 3);
    ASSERT_EQ(v.size(), 3);
    auto it = v.begin();
    ASSERT_EQ(*it++, "ADD R0, R1");
    ASSERT_EQ(*it++, "MOV R2, R0");
    ASSERT_EQ(*it++, "HALT");

    t86.ResumeExecution();
    t86.Wait();
    ASSERT_EQ(t86.GetReason(), DebugEvent::ExecutionEnd);
    t86.ResumeExecution();
    t_os.join();
}

TEST(T86ProcessCpuTest, PokeText) {
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

    auto t86 = T86Process(std::move(tm2), REG_COUNT);
    t86.Wait();
    t86.WriteText(0, {"MOV R1, 1"});
    auto text = t86.ReadText(0, 1);
    ASSERT_EQ(text.size(), 1);
    ASSERT_EQ(text[0], "MOV R1, 1");
    
    t86.Singlestep();
    t86.Wait();
    auto regs = t86.FetchRegisters();
    ASSERT_EQ(regs.at("R0"), 0);
    ASSERT_EQ(regs.at("R1"), 1);

    t86.WriteText(2, {"ADD R1, R1", "MOV R0, R1"});
    text = t86.ReadText(2, 2);
    ASSERT_EQ(text.size(), 2);
    ASSERT_EQ(text[0], "ADD R1, R1");
    ASSERT_EQ(text[1], "MOV R0, R1");

    t86.ResumeExecution();
    t86.Wait();
    ASSERT_EQ(t86.GetReason(), DebugEvent::ExecutionEnd);
    regs = t86.FetchRegisters();
    ASSERT_EQ(regs.at("R0"), 4);
    ASSERT_EQ(regs.at("R1"), 4);
    ASSERT_EQ(regs.at("R2"), 0);
    t86.ResumeExecution();
    t_os.join();
}

TEST(T86ProcessCpuTest, Breakpoint) {
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

    auto t86 = T86Process(std::move(tm2), REG_COUNT);
    t86.Wait();
    t86.WriteText(2, {"BKPT"});
    auto text = t86.ReadText(2, 1);
    ASSERT_EQ(text.size(), 1);
    ASSERT_EQ(text[0], "BKPT");
    
    t86.ResumeExecution();
    t86.Wait();
    ASSERT_EQ(t86.GetReason(), DebugEvent::SoftwareBreakpointHit);
    auto regs = t86.FetchRegisters();
    ASSERT_EQ(regs.at("R0"), 1);
    ASSERT_EQ(regs.at("R1"), 2);

    t86.ResumeExecution();
    t86.Wait();
    ASSERT_EQ(t86.GetReason(), DebugEvent::ExecutionEnd);
    regs = t86.FetchRegisters();
    ASSERT_EQ(regs.at("R0"), 1);
    ASSERT_EQ(regs.at("R1"), 2);
    ASSERT_EQ(regs.at("R2"), 1);
    t86.ResumeExecution();
    t_os.join();
}

TEST(T86ProcessCpuTest, Memory) {
    const size_t REG_COUNT = 3;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
    auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
    auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
    auto program = R"(
.text

0 MOV R0, [1]
1 NOP
2 ADD R0, [2]
3 MOV R2, [2]
4 HALT
)";
    std::thread t_os(RunCPU, std::move(tm1), program, REG_COUNT);
    
    auto t86 = T86Process(std::move(tm2), REG_COUNT);
    t86.Wait();
    t86.WriteMemory(1, {3, 4});
    auto mem = t86.ReadMemory(0, 3);
    ASSERT_EQ(mem.size(), 3);
    ASSERT_EQ(mem[0], 0);
    ASSERT_EQ(mem[1], 3);
    ASSERT_EQ(mem[2], 4);

    t86.ResumeExecution();
    t86.Wait();
    auto regs = t86.FetchRegisters();
    ASSERT_EQ(regs.at("R0"), 7);
    ASSERT_EQ(regs.at("R2"), 4);

    t86.ResumeExecution();
    t_os.join();
}
