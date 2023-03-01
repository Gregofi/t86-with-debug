#include <gtest/gtest.h>
#include "debugger/T86Process.h"
#include "../MockMessenger.h"

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

TEST(T86ProcessTest, ReadRegisters) {
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

TEST(T86ProcessTest, WriteRegisters) {
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


