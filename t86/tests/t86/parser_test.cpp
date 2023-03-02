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
