#include <gtest/gtest.h>
#include <sstream>
#include "debugger/Source/Parser.h"

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
    EXPECT_EQ(info.location_mapping.size(), 4);
    EXPECT_EQ(info.location_mapping.at(0), 3);
    EXPECT_EQ(info.location_mapping.at(1), 3);
    EXPECT_EQ(info.location_mapping.at(2), 4);
    EXPECT_EQ(info.location_mapping.at(3), 5);
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
    EXPECT_EQ(info.location_mapping.size(), 4);
    EXPECT_EQ(info.location_mapping.at(0), 3);
    EXPECT_EQ(info.location_mapping.at(5), 3);
    EXPECT_EQ(info.location_mapping.at(9), 4);
    EXPECT_EQ(info.location_mapping.at(1), 5);
}

TEST(SourceLines, ParsingEmpty) {
    auto program = R"(
.debug_line
.text
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    EXPECT_EQ(info.location_mapping.size(), 0);
}
