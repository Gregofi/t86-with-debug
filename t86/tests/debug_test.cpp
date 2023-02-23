#include <gtest/gtest.h>
#include "t86/os.h"
#include "common/parser.h"
#include "messenger.h"
#include <string>
#include <queue>

class Comms: public Messenger {
public:
    Comms(std::queue<std::string> in, std::vector<std::string>& out): in(std::move(in)), out(out) { }
    void Send(const std::string&s) override {
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
    std::queue<std::string> in;
    std::vector<std::string>& out;
};

using namespace tiny::t86;
#if 0
TEST(DebugTest, SimpleCommands) {
    OS os;
    std::queue<std::string> in({"REASON", "PEEKREGS IP", "CONTINUE", "REASON", "PEEKREGS IP", "CONTINUE"});
    std::vector<std::string> out;

    os.SetDebuggerComms(std::make_unique<Comms>(in, out));

    std::istringstream iss{
R"(
.text

MOV R0, 1
MOV R1, 2
ADD R0, R1
HALT
)"
    };

    Parser parser(iss);
    Program p = parser.Parse();

    os.Run(std::move(p));

    ASSERT_EQ(out.at(0), "STOPPED");
    ASSERT_EQ(out.at(1), "START");
    ASSERT_EQ(out.at(2), "REG:IP VALUE:0");
    ASSERT_EQ(out.at(3), "OK");
    ASSERT_EQ(out.at(4), "STOPPED");
    ASSERT_EQ(out.at(5), "HALT");
    ASSERT_EQ(out.at(6), "REG:IP VALUE:4");
    ASSERT_EQ(out.at(7), "OK");
}
#endif

TEST(DebugTest, SingleStep) {
    OS os;
    std::queue<std::string> in({
        "REASON",
        "PEEKREGS IP",
        "SINGLESTEP",
        "REASON",
        "PEEKREGS IP",
        "SINGLESTEP",
        "PEEKREGS IP",
        "CONTINUE",
        "REASON",
        "PEEKREGS IP",
        "CONTINUE"});
    std::vector<std::string> out;

    os.SetDebuggerComms(std::make_unique<Comms>(in, out));

    std::istringstream iss{
R"(
.text

MOV R0, 1
MOV R1, 2
ADD R0, R1
HALT
)"
    };

    Parser parser(iss);
    Program p = parser.Parse();

    os.Run(std::move(p));
    for (const auto&x: out) std::cerr << x << std::endl;

    ASSERT_EQ(out.at(0), "STOPPED");
    ASSERT_EQ(out.at(1), "START");
    ASSERT_EQ(out.at(2), "REG:IP VALUE:0");
    ASSERT_EQ(out.at(3), "OK");
    ASSERT_EQ(out.at(4), "SINGLESTEP");
    ASSERT_EQ(out.at(5), "REG:IP VALUE:1");
    ASSERT_EQ(out.at(6), "OK");
    ASSERT_EQ(out.at(7), "REG:IP VALUE:2");
    ASSERT_EQ(out.at(8), "STOPPED");
    ASSERT_EQ(out.at(9), "HALT");
    ASSERT_EQ(out.at(10), "REG:IP VALUE:4");
    ASSERT_EQ(out.at(11), "OK");
}
