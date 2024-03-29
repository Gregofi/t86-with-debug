#include <gtest/gtest.h>
#include <sstream>
#include "t86-parser/parser.h"

Token _T(TokenKind kind, size_t row, size_t col) {
    return Token{kind, row, col};
}

TEST(TokenizerTest, OnlyIds) {
    std::istringstream iss("A B   C D");
    Lexer l(iss);
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 0));
    ASSERT_EQ(l.getId(), "A");
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 2));
    ASSERT_EQ(l.getId(), "B");
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 6));
    ASSERT_EQ(l.getId(), "C");
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 8));
    ASSERT_EQ(l.getId(), "D");
    ASSERT_EQ(l.getNext(), _T(TokenKind::END, 0, 9));
}

TEST(TokenizerTest, MixedTokens) {
    std::istringstream iss(
".text 12 MOV[1]; 23 MOV R0 [R0 + 1 + R2 * 2]");
    Lexer l(iss);
    ASSERT_EQ(l.getNext(), _T(TokenKind::DOT, 0, 0));
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 1));
    ASSERT_EQ(l.getId(), "text");
    ASSERT_EQ(l.getNext(), _T(TokenKind::NUM, 0, 6));
    ASSERT_EQ(l.getNumber(), 12);
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 9));
    ASSERT_EQ(l.getId(), "MOV");
    ASSERT_EQ(l.getNext(), _T(TokenKind::LBRACKET, 0, 12));
    ASSERT_EQ(l.getNext(), _T(TokenKind::NUM, 0, 13));
    ASSERT_EQ(l.getNumber(), 1);
    ASSERT_EQ(l.getNext(), _T(TokenKind::RBRACKET, 0, 14));
    ASSERT_EQ(l.getNext(), _T(TokenKind::SEMICOLON, 0, 15));
    ASSERT_EQ(l.getNext(), _T(TokenKind::NUM, 0, 17));
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 20));
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 24));
    ASSERT_EQ(l.getNext(), _T(TokenKind::LBRACKET, 0, 27));
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 28));
    ASSERT_EQ(l.getNext(), _T(TokenKind::PLUS, 0, 31));
    ASSERT_EQ(l.getNext(), _T(TokenKind::NUM, 0, 33));
    ASSERT_EQ(l.getNext(), _T(TokenKind::PLUS, 0, 35));
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 37));
    ASSERT_EQ(l.getNext(), _T(TokenKind::TIMES, 0, 40));
    ASSERT_EQ(l.getNext(), _T(TokenKind::NUM, 0, 42));
    ASSERT_EQ(l.getNext(), _T(TokenKind::RBRACKET, 0, 43));
    ASSERT_EQ(l.getNext(), _T(TokenKind::END, 0, 44));
}

TEST(TokenizerTest, Minus) {
    std::istringstream iss("MOV [-1], R0");
    Lexer l(iss);
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 0));
    ASSERT_EQ(l.getNext(), _T(TokenKind::LBRACKET, 0, 4));
    ASSERT_EQ(l.getNext(), _T(TokenKind::MINUS, 0, 5));
    ASSERT_EQ(l.getNext(), _T(TokenKind::NUM, 0, 6));
    ASSERT_EQ(l.getNumber(), 1);
    ASSERT_EQ(l.getNext(), _T(TokenKind::RBRACKET, 0, 7));
    ASSERT_EQ(l.getNext(), _T(TokenKind::COMMA, 0, 8));
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 10));
}

TEST(TokenizerTest, String) {
    std::istringstream iss("\"Hello\" 1 2 [R0]");
    Lexer l(iss);
    ASSERT_EQ(l.getNext(), _T(TokenKind::STRING, 0, 0));
    ASSERT_EQ(l.getStr(), "Hello");
    ASSERT_EQ(l.getNext(), _T(TokenKind::NUM, 0, 8));
    ASSERT_EQ(l.getNext(), _T(TokenKind::NUM, 0, 10));
    ASSERT_EQ(l.getNext(), _T(TokenKind::LBRACKET, 0, 12));
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 13));
    ASSERT_EQ(l.getNext(), _T(TokenKind::RBRACKET, 0, 15));
}

TEST(TokenizerTest, MultilineInput) {
    std::istringstream iss("\"Hello\" 1\n 1 2\n[R0\n]\n");
    Lexer l(iss);
    ASSERT_EQ(l.getNext(), _T(TokenKind::STRING, 0, 0));
    ASSERT_EQ(l.getNext(), _T(TokenKind::NUM, 0, 8));
    ASSERT_EQ(l.getNext(), _T(TokenKind::NUM, 1, 1));
    ASSERT_EQ(l.getNext(), _T(TokenKind::NUM, 1, 3));
    ASSERT_EQ(l.getNext(), _T(TokenKind::LBRACKET, 2, 0));
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 2, 1));
    ASSERT_EQ(l.getNext(), _T(TokenKind::RBRACKET, 3, 0));
    ASSERT_EQ(l.getNext(), _T(TokenKind::END, 4, 0));
}

TEST(TokenizerTest, UnterminatedString) {
    std::istringstream iss("1 \"Hello 2");
    Lexer l(iss);
    ASSERT_EQ(l.getNext(), _T(TokenKind::NUM, 0, 0));
    ASSERT_THROW({l.getNext();}, ParserError);
}

TEST(TokenizerTest, Floats) {
    std::istringstream iss(".data 1 2.0 3.14 5.8 2. 3");
    Lexer l(iss);
    ASSERT_EQ(l.getNext(), _T(TokenKind::DOT, 0, 0));
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 1));
    ASSERT_EQ(l.getNext(), _T(TokenKind::NUM, 0, 6));
    ASSERT_EQ(l.getNumber(), 1);
    ASSERT_EQ(l.getNext(), _T(TokenKind::FLOAT, 0, 8));
    ASSERT_EQ(l.getFloat(), 2.0);
    ASSERT_EQ(l.getNext(), _T(TokenKind::FLOAT, 0, 12));
    ASSERT_EQ(l.getFloat(), 3.14);
    ASSERT_EQ(l.getNext(), _T(TokenKind::FLOAT, 0, 17));
    ASSERT_EQ(l.getFloat(), 5.8);
    ASSERT_EQ(l.getNext(), _T(TokenKind::FLOAT, 0, 21));
    ASSERT_EQ(l.getFloat(), 2.);
    ASSERT_EQ(l.getNext(), _T(TokenKind::NUM, 0, 24));
    ASSERT_EQ(l.getNumber(), 3);
}

TEST(TokenizerTest, Floats2) {
    std::istringstream iss("MOV F0, 3.14");
    Lexer l(iss);
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 0));
    ASSERT_EQ(l.getNext(), _T(TokenKind::ID, 0, 4));
    ASSERT_EQ(l.getNext(), _T(TokenKind::COMMA, 0, 6));
    ASSERT_EQ(l.getNext(), _T(TokenKind::FLOAT, 0, 8));
    ASSERT_EQ(l.getFloat(), 3.14);
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
    ASSERT_EQ(p.data().size(), strlen("Hello, World!") + 1);
    auto&& d = p.data();
    std::transform(d.begin(), d.end(), std::back_inserter(content),
                   [](auto &&c) { return static_cast<char>(c); });
    ASSERT_STREQ(content.c_str(), "Hello, World!");
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
    ASSERT_EQ(p.data().size(), strlen("Hello, World!") + 3);
    auto&& d = p.data();
    std::transform(d.begin(), d.end(), std::back_inserter(content),
                   [](auto &&c) { return static_cast<char>(c); });
    ASSERT_STREQ(content.c_str(), "Hello");
    ASSERT_STREQ(content.c_str() + strlen("Hello") + 1, ", ");
    ASSERT_STREQ(content.c_str() + strlen("Hello, ") + 2, "World!");
}

TEST(ParserTest, MixStringsAndNumbers) {
    auto program = R"(
.data
"Hell"
111
", "
87 111
"rl" 100 33 0
.text
0 MOV R0, [0]
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 1);
    std::string content;
    ASSERT_EQ(p.data().size(), strlen("Hello, World!") + 4);
    auto&& d = p.data();
    std::transform(d.begin(), d.end(), std::back_inserter(content),
                   [](auto &&c) { return static_cast<char>(c); });
    auto *ptr = content.c_str();
    ASSERT_STREQ(ptr, "Hell");
    ptr += strlen("Hell") + 1;
    ASSERT_STREQ(ptr, "o, ");
    ptr += strlen("o, ") + 1;
    ASSERT_STREQ(ptr, "Worl");
    ptr += strlen("Worl") + 1;
    ASSERT_STREQ(ptr, "d!");
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
    ASSERT_EQ(p.data().size(), strlen("Hello\nWorld!") + 1);
    auto&& d = p.data();
    std::transform(d.begin(), d.end(), std::back_inserter(content),
                   [](auto &&c) { return static_cast<char>(c); });
    ASSERT_STREQ(content.c_str(), "Hello\nWorld!");
    ASSERT_EQ(content.back(), 0);
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
    ASSERT_EQ(p.data().size(), strlen("Hello, World!") + 5);
    auto&& d = p.data();
    std::transform(d.begin(), d.end(), std::back_inserter(content),
                   [](auto &&c) { return static_cast<char>(c); });
    auto ptr = content.c_str();
    ASSERT_STREQ(ptr, "Hello");
    ptr += strlen("Hello") + 1;
    ASSERT_STREQ(ptr, ", ");
    ptr += strlen(", ") + 1;
    ASSERT_STREQ(ptr, "Wor");
    ptr += strlen("Wor") + 1;
    ASSERT_STREQ(ptr, "ld");
    ptr += strlen("ld") + 1;
    ASSERT_STREQ(ptr, "!");
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
    ASSERT_EQ(p.data().size(), strlen("Hello\nWorld!") + 1);
    auto&& d = p.data();
    std::transform(d.begin(), d.end(), std::back_inserter(content),
                   [](auto &&c) { return static_cast<char>(c); });
    ASSERT_STREQ(content.c_str(), "Hello\nWorld!");
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

void Parse(std::string_view program) {
    std::istringstream iss{std::string(program)};
    Parser parser(iss);
    parser.Parse();
}

TEST(ParserTest, AddressingOperands) {
    auto program = R"(
.text
0 ADD R0, 2
0 ADD R0, R1
0 ADD R0, R1 + 3
0 ADD R0, [2]
0 ADD R0, [R1]
0 ADD R0, [R1 + 1]
)";
    Parse(program);
}

TEST(ParserTest, BadBinaryInstructions) {
    auto program = R"(
.text
0 MOV ADD 1, 2
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 MOV ADD 1
)";
    ASSERT_THROW({Parse(program);}, ParserError);

    program = R"(
.text
0 MOV ADD 1, [1 + 1]
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 MOV ADD 1, [R0 * 2]
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 MOV ADD [2], 3
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 MOV ADD R0, 3.0
)";
    ASSERT_THROW({Parse(program);}, ParserError);
}

TEST(ParserTest, BadUnaryInstructions) {
    auto program = R"(
.text
0 JMP [0]
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 JMP [R1]
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 POP 0
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 PUSH [0]
)";
    ASSERT_THROW({Parse(program);}, ParserError);
}

TEST(ParserTest, FloatInstructions) {
auto program = R"(
.text

0 FADD F0, 3.14
1 FSUB F0, F1
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 2);
}

TEST(ParserTest, LeaMemory) {
auto program = R"(
.text

0 LEA R0, [R1 + 1]
0 LEA R0, [R1 + R2]
0 LEA R0, [R1 * 2]
0 LEA R0, [R1 + 1 + R2]
0 LEA R0, [R1 + R2 * 2]
0 LEA R0, [R1 + 1 + R2 * 2]
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 6);

    program = R"(
.text
0 LEA R0, [R1]
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 LEA R0, [1]
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 LEA R0, R1
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 LEA R0, [R1 + ]
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 LEA R0, [R1 * ]
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 LEA R0, [R1 + 1 + ]
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 LEA R0, [R1 + 1 + R2 * ]
)";
    ASSERT_THROW({Parse(program);}, ParserError);
}

TEST(ParserTest, Mov) {
auto program = R"(
.text

MOV R0, R1
MOV R0, 3
MOV R0, [4]
MOV R0, [R1]
MOV R0, [R1 + 1]
MOV R0, [R1 + R2]
MOV R0, [R1 * 2]
MOV R0, [R1 + 1 + R2]
MOV R0, [R1 + R2 * 2]
MOV R0, [R1 + 1 + R2 * 2]
MOV R0, [R1 + 1 + R2 * 2]

MOV F0, 3.14
MOV F0, F1
MOV F0, R1
MOV F0, [2]
MOV F0, [R1]

MOV [1], R1
MOV [1], F1
MOV [1], 2

MOV [R0], R1
MOV [R1], F1
MOV [R1], 2

MOV [R0 + 1], R1
MOV [R0 + 1], F1
MOV [R0 + 1], 2

MOV [R0 * 2], R1
MOV [R0 * 2], F1
MOV [R0 * 2], 2

MOV [R0 + R1], R1
MOV [R0 + R1], F1
MOV [R0 + R1], 2

MOV [R0 + R1 * 3], R1
MOV [R0 + R1 * 3], F1
MOV [R0 + R1 * 3], 2

MOV [R0 + 1 + R2], R1
MOV [R0 + 1 + R2], F1
MOV [R0 + 1 + R2], 2

MOV [R0 + 1 + R2 * 4], R1
MOV [R0 + 1 + R2 * 4], F1
MOV [R0 + 1 + R2 * 4], 2
)";
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    ASSERT_EQ(p.instructions().size(), 40);

    program = R"(
.text
0 MOV R0, R1 + 1
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 MOV [1], [1]
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 MOV [R0], 3.14
)";
    ASSERT_THROW({Parse(program);}, ParserError);
    program = R"(
.text
0 MOV [R1 + R2 * 3], [R4]
)";
    ASSERT_THROW({Parse(program);}, ParserError);
}
