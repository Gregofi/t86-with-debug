#include <gtest/gtest.h>

#include "t86/os.h"
#include "common/parser.h"
#include "messenger.h"
#include "MockMessenger.h"

#include <string>
#include <queue>

using namespace tiny::t86;

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

    auto it = out.begin();
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "START");
    ASSERT_EQ(*it++, "REG:IP VALUE:0");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "HALT");
    ASSERT_EQ(*it++, "REG:IP VALUE:4");
    ASSERT_EQ(*it++, "OK");
}

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
        });
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

    auto it = out.begin();
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "START");
    ASSERT_EQ(*it++, "REG:IP VALUE:0");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "SINGLESTEP");
    ASSERT_EQ(*it++, "REG:IP VALUE:1");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "REG:IP VALUE:2");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "HALT");
    ASSERT_EQ(*it++, "REG:IP VALUE:4");
}

TEST(DebugTest, IPManipulation) {
    OS os;
    std::queue<std::string> in({
        "REASON",
        // Step to address 2
        "SINGLESTEP",
        "SINGLESTEP",
        "PEEKREGS IP",
        "PEEKREGS R0",
        // Add R0 += R1 and "step back"
        "SINGLESTEP",
        "PEEKREGS R0",
        "POKEREGS IP 2",
        // Add R0 += R1 and "step back"
        "SINGLESTEP",
        "PEEKREGS R0",
        "POKEREGS IP 2",
        // Step just before halt
        "SINGLESTEP",
        "SINGLESTEP",
        "PEEKREGS R2",
    });

    std::vector<std::string> out;

    os.SetDebuggerComms(std::make_unique<Comms>(in, out));

    std::istringstream iss{
R"(
.text

0 MOV R0, 1
1 MOV R1, 2
2 ADD R0, R1
3 MOV R2, R0
4 HALT
)"
    };

    Parser parser(iss);
    Program p = parser.Parse();

    os.Run(std::move(p));

    auto it = out.begin();
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "START");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "REG:IP VALUE:2");
    ASSERT_EQ(*it++, "REG:R0 VALUE:1");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "REG:R0 VALUE:3");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "REG:R0 VALUE:5");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "REG:R2 VALUE:7");
}

TEST(DebugTest, Breakpoint) {
    OS os;
    std::queue<std::string> in({
            "POKETEXT 2 BKPT",
            "PEEKTEXT 2",
            "CONTINUE",
            "PEEKREGS R0",
            "POKEREGS IP 2",
            "POKETEXT 2 ADD R0, R1",
            "SINGLESTEP",
            "PEEKREGS R0",
            "CONTINUE",
            "REASON",

    });

    std::vector<std::string> out;

    os.SetDebuggerComms(std::make_unique<Comms>(in, out));

    std::istringstream iss{
R"(
.text

0 MOV R0, 1
1 MOV R1, 2
2 ADD R0, R1
3 MOV R2, R0
4 HALT
)"
    };

    Parser parser(iss);
    Program p = parser.Parse();

    os.Run(std::move(p));

    auto it = out.begin();
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "TEXT:2 VALUE:BKPT");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "REG:R0 VALUE:1");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "REG:R0 VALUE:3");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "HALT");
}

TEST(DebugTest, Sizes) {
    OS os;
    std::queue<std::string> in({
        "REGCOUNT",
        "TEXTSIZE",
        "DATASIZE",
    });

    std::vector<std::string> out;

    os.SetDebuggerComms(std::make_unique<Comms>(in, out));

    std::istringstream iss{
R"(
.text

0 MOV R0, 1
1 MOV R1, 2
2 ADD R0, R1
3 MOV R2, R0
4 HALT
)"
    };

    Parser parser(iss);
    Program p = parser.Parse();

    os.Run(std::move(p));
    
    auto it = out.begin();
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "REGCOUNT:10");
    ASSERT_EQ(*it++, "TEXTSIZE:5");
    ASSERT_EQ(*it++, "DATASIZE:1024");
}
