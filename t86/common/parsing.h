#pragma once
#include <optional>
#include <variant>
#include <iostream>
#include <cstring>
#include <string>
#include <string_view>
#include "fmt/core.h"

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
    LBRACKET,
    RBRACKET,
    END,
    SEMICOLON,
    PLUS,
    TIMES,
    COMMA,
    STRING,
    FLOAT,
    DOUBLEDOT,
};

struct Token {
    TokenKind kind;
    size_t row;
    size_t col;
    friend bool operator==(const Token& t1, const Token& t2) = default;
};



/// Parses text representation of T86 into in-memory representation
class Lexer {
public:
    explicit Lexer(std::istream& input) noexcept : input(input), lookahead(input.get()) { }

    void ParseString() {
        str.clear();
        while (GetChar() != '"') {
            if (lookahead == EOF) {
                throw ParserError("Unterminated string!");
            } else if (lookahead == '\\') {
                GetChar();
                if (lookahead == 'n') {
                    str += '\n';
                } else if (lookahead == 't') {
                    str += '\t';
                } else if (lookahead == '\\') {
                    str += '\\';
                } else if (lookahead == '\"') {
                    str += '\"';
                } else {
                    throw ParserError(fmt::format("Unknown escape sequence: '\\{}'", lookahead));
                }
            } else {
                str += lookahead;
            }
        }
        GetChar();
    }

    /// If set then characters which do not match any tokens are
    /// simply discarded.
    void SetIgnoreMode(bool on) {
        ignore = on;
    }

    TokenKind ParseNumber() {
        int neg = lookahead == '-' ? -1 : 1;
        if (neg == -1) {
            lookahead = GetChar();
        }
        bool is_float = false;
        std::string num{lookahead};
        while (true) {
            lookahead = GetChar();
            if (lookahead == '.') {
                is_float = true;

            } else if (!isdigit(lookahead)) {
                break;
            }
            num += lookahead;
        }
        if (is_float) {
            float_number = neg * std::stod(num);
            return TokenKind::FLOAT;
        } else {
            number = neg * std::stoi(num);
            return TokenKind::NUM;
        }
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

    Token MakeToken(TokenKind kind) {
        Token tok{kind, tok_begin_row, tok_begin_col};
        return tok;
    }

    Token getNext() {
        if (lookahead == '#') {
            while (lookahead != EOF && lookahead != '\n') {
                GetChar();
            }
            if (lookahead == '\n') {
                GetChar();
            }
            return getNext();
        } else if (isspace(lookahead)) {
            GetChar();
            return getNext();
        }

        RecordTokLoc();
        if (lookahead == EOF) {
            return MakeToken(TokenKind::END);
        } else if (lookahead == ';') {
            GetChar();
            return MakeToken(TokenKind::SEMICOLON);
        } else if (lookahead == ',') {
            GetChar();
            return MakeToken(TokenKind::COMMA);
        } else if (lookahead == '[') {
            GetChar();
            return MakeToken(TokenKind::LBRACKET);
        } else if (lookahead == ']') {
            GetChar();
            return MakeToken(TokenKind::RBRACKET);
        } else if (lookahead == '+') {
            GetChar();
           return MakeToken(TokenKind::PLUS);
        } else if (lookahead == ':') {
            GetChar();
            return MakeToken(TokenKind::DOUBLEDOT);
        } else if (lookahead == '*') {
            GetChar();
            return MakeToken(TokenKind::TIMES);
        } else if (lookahead == '.') {
            GetChar();
            return MakeToken(TokenKind::DOT);
        } else if (lookahead == '"') {
            ParseString();
            return MakeToken(TokenKind::STRING);
        } else if (isdigit(lookahead) || lookahead == '-') {
            // Can be either int or float
            return MakeToken(ParseNumber());
        } else if (isalpha(lookahead) || lookahead == '_') { // identifier
            ParseIdentifier();
            return MakeToken(TokenKind::ID);
        } else {
            if (ignore) {
                GetChar();
                return getNext();
            } else {
                throw ParserError(fmt::format("{}:{}:No token beginning with '{}'",
                                              row, col, lookahead));
            }
        }
    }

    std::string getId() const { return id; }
    int getNumber() const noexcept { return number; }
    double getFloat() const noexcept { return float_number; }
    std::string getStr() const noexcept { return str; }
private:
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

