#include <gtest/gtest.h>
#include <sstream>
#include "common/parser.h"

TEST(TokenizerTest, OnlyIds) {
    std::istringstream iss("A B C D");
    Lexer l(iss);
    ASSERT_EQ(l.getNext(), Token::ID);
    ASSERT_EQ(l.getId(), "A");
    ASSERT_EQ(l.getNext(), Token::ID);
    ASSERT_EQ(l.getId(), "B");
    ASSERT_EQ(l.getNext(), Token::ID);
    ASSERT_EQ(l.getId(), "C");
    ASSERT_EQ(l.getNext(), Token::ID);
    ASSERT_EQ(l.getId(), "D");
    ASSERT_EQ(l.getNext(), Token::END);
}

TEST(TokenizerTest, MixedTokens) {
    std::istringstream iss(".text 12 MOV[1]; 23 MOV R0 [R0 + 1 + R2 * 2]");
    Lexer l(iss);
    ASSERT_EQ(l.getNext(), Token::DOT);
    ASSERT_EQ(l.getNext(), Token::ID);
    ASSERT_EQ(l.getId(), "text");
    ASSERT_EQ(l.getNext(), Token::NUM);
    ASSERT_EQ(l.getNumber(), 12);
    ASSERT_EQ(l.getNext(), Token::ID);
    ASSERT_EQ(l.getId(), "MOV");
    ASSERT_EQ(l.getNext(), Token::LBRACKET);
    ASSERT_EQ(l.getNext(), Token::NUM);
    ASSERT_EQ(l.getNumber(), 1);
    ASSERT_EQ(l.getNext(), Token::RBRACKET);
    ASSERT_EQ(l.getNext(), Token::SEMICOLON);
    ASSERT_EQ(l.getNext(), Token::NUM);
    ASSERT_EQ(l.getNext(), Token::ID);
    ASSERT_EQ(l.getNext(), Token::ID);
    ASSERT_EQ(l.getNext(), Token::LBRACKET);
    ASSERT_EQ(l.getNext(), Token::ID);
    ASSERT_EQ(l.getNext(), Token::PLUS);
    ASSERT_EQ(l.getNext(), Token::NUM);
    ASSERT_EQ(l.getNext(), Token::PLUS);
    ASSERT_EQ(l.getNext(), Token::ID);
    ASSERT_EQ(l.getNext(), Token::TIMES);
    ASSERT_EQ(l.getNext(), Token::NUM);
    ASSERT_EQ(l.getNext(), Token::RBRACKET);
    ASSERT_EQ(l.getNext(), Token::END);
}

TEST(TokenizerTest, Minus) {
    std::istringstream iss("MOV [-1], R0");
    Lexer l(iss);
    ASSERT_EQ(l.getNext(), Token::ID);
    ASSERT_EQ(l.getNext(), Token::LBRACKET);
    ASSERT_EQ(l.getNext(), Token::NUM);
    ASSERT_EQ(l.getNumber(), -1);
    ASSERT_EQ(l.getNext(), Token::RBRACKET);
    ASSERT_EQ(l.getNext(), Token::COMMA);
    ASSERT_EQ(l.getNext(), Token::ID);
}

TEST(ParserTest, Minuses) {
auto program = R"(
.text

0 MOV R0, [BP + -1]
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 1);
}

TEST(ParserTest, SkipSection1) {
    auto program = R"(
.text
0 MOV R0, [BP + -1]
.unknown_nonsensical_section_name
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 1);
}

TEST(ParserTest, SkipSection2) {
    auto program = R"(
.text
0 MOV R0, [BP + -1]
.unknown_nonsensical_section_name
blablabla
Very interesting
section
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 1);
}

TEST(ParserTest, SkipSection3) {
    auto program = R"(
.unknown_nonsensical_section_name0
.text
0 MOV R0, [BP + -1]
.unknown_nonsensical_section_name1
ABCD
.unknown_nonsensical_section_name2
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 1);
}

TEST(ParserTest, EmptyData) {
    auto program = R"(
.data
.text
0 MOV R0, [0]
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 1);
}

TEST(ParserTest, DataWithNumbers) {
    auto program = R"(
.data
1
2
3 4 5
    6
    7
.text
0 MOV R0, [0]
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 1);
    ASSERT_EQ(p.data().size(), 7);
    ASSERT_EQ(p.data()[0], 1);
    ASSERT_EQ(p.data()[1], 2);
    ASSERT_EQ(p.data()[2], 3);
    ASSERT_EQ(p.data()[3], 4);
    ASSERT_EQ(p.data()[4], 5);
    ASSERT_EQ(p.data()[5], 6);
    ASSERT_EQ(p.data()[6], 7);
}

TEST(ParserTest, DataWithNegativeNumbers) {
    auto program = R"(
.data
-1 -2 3 -4 -5
-1 2 -3
.text
0 MOV R0, [0]
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 1);
    ASSERT_EQ(p.data().size(), 8);
    ASSERT_EQ(p.data()[0], -1);
    ASSERT_EQ(p.data()[1], -2);
    ASSERT_EQ(p.data()[2], 3);
    ASSERT_EQ(p.data()[3], -4);
    ASSERT_EQ(p.data()[4], -5);
    ASSERT_EQ(p.data()[5], -1);
    ASSERT_EQ(p.data()[6], 2);
    ASSERT_EQ(p.data()[7], -3);
}

TEST(ParserTest, String) {
    auto program = R"(
.data
"Hello, World!"
.text
0 MOV R0, [0]
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 1);
    std::string content;
    ASSERT_EQ(p.data().size(), strlen("Hello, World!"));
    auto&& d = p.data();
    std::transform(d.begin(), d.end(), std::back_inserter(content),
                   [](auto &&c) { return static_cast<char>(c); });
    ASSERT_EQ(content, "Hello, World!");
}

TEST(ParserTest, MultipleStrings) {
    auto program = R"(
.data
"Hello"
", "

"World!"
.text
0 MOV R0, [0]
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 1);
    std::string content;
    ASSERT_EQ(p.data().size(), strlen("Hello, World!"));
    auto&& d = p.data();
    std::transform(d.begin(), d.end(), std::back_inserter(content),
                   [](auto &&c) { return static_cast<char>(c); });
    ASSERT_EQ(content, "Hello, World!");
}

TEST(ParserTest, MixStringsAndNumbers) {
    auto program = R"(
.data
"Hell"
111
", "
87 111
"rl" 100 33
.text
0 MOV R0, [0]
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 1);
    std::string content;
    ASSERT_EQ(p.data().size(), strlen("Hello, World!"));
    auto&& d = p.data();
    std::transform(d.begin(), d.end(), std::back_inserter(content),
                   [](auto &&c) { return static_cast<char>(c); });
    ASSERT_EQ(content, "Hello, World!");
}

TEST(ParserTest, MultilineString) {
    auto program = R"(
.data
"Hello
World!"
.text
0 MOV R0, [0]
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 1);
    std::string content;
    ASSERT_EQ(p.data().size(), strlen("Hello\nWorld!"));
    auto&& d = p.data();
    std::transform(d.begin(), d.end(), std::back_inserter(content),
                   [](auto &&c) { return static_cast<char>(c); });
    ASSERT_EQ(content, "Hello\nWorld!");
}

TEST(ParserTest, MultipleStringsOnSameLine) {
    auto program = R"(
.data
"Hello" ", " "Wor" 
"ld" "!"
.text
0 MOV R0, [0]
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 1);
    std::string content;
    ASSERT_EQ(p.data().size(), strlen("Hello, World!"));
    auto&& d = p.data();
    std::transform(d.begin(), d.end(), std::back_inserter(content),
                   [](auto &&c) { return static_cast<char>(c); });
    ASSERT_EQ(content, "Hello, World!");
}

TEST(ParserTest, EscapeSequence) {
    auto program = R"(
.data
"Hello\nWorld!"
.text
0 MOV R0, [0]
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 1);
    std::string content;
    ASSERT_EQ(p.data().size(), strlen("Hello\nWorld!"));
    auto&& d = p.data();
    std::transform(d.begin(), d.end(), std::back_inserter(content),
                   [](auto &&c) { return static_cast<char>(c); });
    ASSERT_EQ(content, "Hello\nWorld!");
}

TEST(ParserTest, WrongEscapeSequence) {
    auto program = R"(
.data
"Hello\xWorld!"
.text
0 MOV R0, [0]
)";
    std::istringstream iss{program};
    Parser parser(iss);
    ASSERT_THROW({
        auto p = parser.Parse();
        }, ParserError);
}
