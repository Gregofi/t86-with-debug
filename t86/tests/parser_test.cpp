#include <gtest/gtest.h>
#include <sstream>
#include "../t86-cli/parser.h"

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
    std::istringstream iss(".data 12 MOV[1]; 23 MOV R0 [R0 + 1 + R2 * 2]");
    Lexer l(iss);
    ASSERT_EQ(l.getNext(), Token::DOT);
    ASSERT_EQ(l.getNext(), Token::ID);
    ASSERT_EQ(l.getId(), "data");
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