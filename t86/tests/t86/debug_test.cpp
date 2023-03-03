#include <gtest/gtest.h>

#include "../MockMessenger.h"
#include "messenger.h"
#include "t86-parser/parser.h"
#include "t86/os.h"

#include <queue>
#include <string>

using namespace tiny::t86;

TEST(DebugTest, SimpleCommands) {
    OS os(2);
    std::queue<std::string> in({ "REASON", "PEEKREGS", "CONTINUE", "REASON",
        "PEEKREGS", "CONTINUE" });
    std::vector<std::string> out;

    os.SetDebuggerComms(std::make_unique<Comms>(in, out));

    std::istringstream iss {
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
    ASSERT_EQ(*it++,
        "IP:0\nBP:1024\nSP:1024\n"
        "R0:0\nR1:0\n");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "HALT");
    ASSERT_EQ(*it++,
        "IP:4\nBP:1024\nSP:1024\n"
        "R0:3\nR1:2\n");
    ASSERT_EQ(*it++, "OK");
}

TEST(DebugTest, SingleStep) {
    OS os(2);
    std::queue<std::string> in({
        "REASON",
        "PEEKREGS",
        "SINGLESTEP",
        "REASON",
        "PEEKREGS",
        "SINGLESTEP",
        "PEEKREGS",
        "CONTINUE",
        "REASON",
        "PEEKREGS",
    });
    std::vector<std::string> out;

    os.SetDebuggerComms(std::make_unique<Comms>(in, out));

    std::istringstream iss {
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
    ASSERT_EQ(*it++,
        "IP:0\nBP:1024\nSP:1024\n"
        "R0:0\nR1:0\n");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "SINGLESTEP");
    ASSERT_EQ(*it++,
        "IP:1\nBP:1024\nSP:1024\n"
        "R0:1\nR1:0\n");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++,
        "IP:2\nBP:1024\nSP:1024\n"
        "R0:1\nR1:2\n");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "HALT");
    ASSERT_EQ(*it++,
        "IP:4\nBP:1024\nSP:1024\n"
        "R0:3\nR1:2\n");
}

TEST(DebugTest, IPManipulation) {
    OS os(3);
    std::queue<std::string> in({
        "REASON",
        // Step to address 2
        "SINGLESTEP",
        "SINGLESTEP",
        "PEEKREGS",
        // Add R0 += R1 and "step back"
        "SINGLESTEP",
        "PEEKREGS",
        "POKEREGS IP 2",
        // Add R0 += R1 and "step back"
        "SINGLESTEP",
        "PEEKREGS",
        "POKEREGS IP 2",
        // Step just before halt
        "SINGLESTEP",
        "SINGLESTEP",
        "PEEKREGS",
    });

    std::vector<std::string> out;

    os.SetDebuggerComms(std::make_unique<Comms>(in, out));

    std::istringstream iss {
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
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++,
        "IP:2\nBP:1024\nSP:1024\n"
        "R0:1\nR1:2\nR2:0\n");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++,
        "IP:3\nBP:1024\nSP:1024\n"
        "R0:3\nR1:2\nR2:0\n");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++,
        "IP:3\nBP:1024\nSP:1024\n"
        "R0:5\nR1:2\nR2:0\n");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++,
        "IP:4\nBP:1024\nSP:1024\n"
        "R0:7\nR1:2\nR2:7\n");
}

TEST(DebugTest, Breakpoint) {
    OS os(3);
    std::queue<std::string> in({
        "POKETEXT 2 BKPT",
        "PEEKTEXT 2 1",
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

    std::istringstream iss {
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
    ASSERT_EQ(*it++, "BKPT\n");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++,
        "IP:3\nBP:1024\nSP:1024\n"
        "R0:1\nR1:2\nR2:0\n");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++,
        "IP:3\nBP:1024\nSP:1024\n"
        "R0:3\nR1:2\nR2:0\n");
    ASSERT_EQ(*it++, "OK");
    ASSERT_EQ(*it++, "STOPPED");
    ASSERT_EQ(*it++, "HALT");
}

TEST(DebugTest, PeekDataText) {
    OS os(3);
    std::queue<std::string> in({
        "CONTINUE",
        "PEEKTEXT 2 3",
        "PEEKDATA 1 3",
        "PEEKREGS",
    });

    std::vector<std::string> out;

    os.SetDebuggerComms(std::make_unique<Comms>(in, out));

    std::istringstream iss {
        R"(
.text

0 MOV [1], 1
1 MOV [3], 2
2 ADD R0, [1]
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
    ASSERT_EQ(*it++, "STOPPED");
    EXPECT_EQ(*it++, "ADD R0, [1]\nMOV R2, R0\nHALT\n");
    EXPECT_EQ(*it++, "1\n0\n2\n");
    EXPECT_EQ(*it++,
        "IP:5\nBP:1024\nSP:1024\n"
        "R0:1\nR1:0\nR2:1\n");
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

    std::istringstream iss {
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
