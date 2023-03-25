#pragma once
#include <optional>
#include <variant>
#include <iostream>
#include <sstream>
#include <cstring>
#include <string>
#include <string_view>
#include "fmt/core.h"
#include "logger.h"

class ParserError : public std::exception {
public:
    ParserError(std::string message) : message(std::move(message)) { }
    const char* what() const noexcept override {
        return message.c_str();
    }
private:
    std::string message;
};

enum class TokenKind {
    ID,
    DOT,
    NUM,
    LBRACKET, // [
    RBRACKET,
    LBRACE, // {
    RBRACE,
    LPAREN, // (
    RPAREN,
    END,
    SEMICOLON,
    BANG,
    PLUS,
    MINUS,
    TIMES,
    LESS,
    DOLLAR,
    SLASH,
    GREATER,
    COMMA,
    STRING,
    ASSIGN,
    EQ,
    NEQ,
    GEQ,
    LEQ,
    FLOAT,
    DOUBLEDOT,
    BACKTICK,
    ARROW, // ->
    LAND,
    LOR,
    AND,
    OR,
    XOR,
    LSHIFT,
    RSHIFT,
    MOD,
};

struct Token {
    TokenKind kind;
    size_t row;
    size_t col;
    friend bool operator==(const Token& t1, const Token& t2) = default;
};



/// General purpose lexer.
class Lexer {
public:
    explicit Lexer(std::istream& input) noexcept : input(input), lookahead(input.get()) { }

    void ParseString();

    /// If set then characters which do not match any tokens are
    /// simply discarded.
    void SetIgnoreMode(bool on) {
        ignore = on;
    }

    Token getNext();

    /// Reads the rest of the input into a string.
    std::string RawMode() {
        std::stringstream buffer;
        buffer << input.rdbuf();
        log_debug("Raw parsed >{}<", buffer.str());
        return buffer.str();
    }

    std::string getId() const { return id; }
    int getNumber() const noexcept { return number; }
    double getFloat() const noexcept { return float_number; }
    std::string getStr() const noexcept { return str; }
private:
    TokenKind ParseNumber();

    Token MakeToken(TokenKind kind) {
        Token tok{kind, tok_begin_row, tok_begin_col};
        return tok;
    }

    void ParseIdentifier() {
        std::string str{lookahead};
        while (true) {
            GetChar();
            if (!isalnum(lookahead) && lookahead != '_') {
                break;
            }
            str += lookahead;
        }
        id = str;
    }

    char GetChar() {
        if (lookahead == '\n') {
            row += 1;
            col = 0;
        } else {
            col += 1;
        }
        lookahead = input.get();
        return lookahead;
    }

    void RecordTokLoc() {
        tok_begin_row = row;
        tok_begin_col = col;
    }

    size_t row{0};
    size_t col{0};

    size_t tok_begin_row{0};
    size_t tok_begin_col{0};

    int number{-1};
    double float_number{-1};
    std::string id;
    std::string str;

    std::istream& input;
    char lookahead;

    bool ignore{false};
};

