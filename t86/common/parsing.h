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
            float_number = std::stod(num);
            return TokenKind::FLOAT;
        } else {
            number = std::stoi(num);
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
        } else if (lookahead == '{') {
            GetChar();
            return MakeToken(TokenKind::LBRACE);
        } else if (lookahead == '}') {
            GetChar();
            return MakeToken(TokenKind::RBRACE);
        } else if (lookahead == '(') {
            GetChar();
            return MakeToken(TokenKind::LPAREN);
        } else if (lookahead == ')') {
            GetChar();
            return MakeToken(TokenKind::RPAREN);
        } else if (lookahead == '!') {
            GetChar();
            if (lookahead == '=') {
                GetChar();
                return MakeToken(TokenKind::NEQ);
            }
            return MakeToken(TokenKind::BANG);
        } else if (lookahead == '=') {
            GetChar();
            if (lookahead == '=') {
                GetChar();
                MakeToken(TokenKind::EQ);
            }
            return MakeToken(TokenKind::ASSIGN);
        } else if (lookahead == '%') {
            GetChar();
            return MakeToken(TokenKind::MOD);
        } else if (lookahead == '+') {
            GetChar();
           return MakeToken(TokenKind::PLUS);
        } else if (lookahead == '-') {
            GetChar();
            if (lookahead == '>') {
                GetChar();
                return MakeToken(TokenKind::ARROW);
            }
           return MakeToken(TokenKind::MINUS);
        } else if (lookahead == '>') {
            GetChar();
            if (lookahead == '=') {
                GetChar();
                return MakeToken(TokenKind::GEQ);
            } else if (lookahead == '>') {
                GetChar();
                return MakeToken(TokenKind::RSHIFT);
            }
            return MakeToken(TokenKind::GREATER);
        } else if (lookahead == '<') {
            GetChar();
            if (lookahead == '=') {
                GetChar();
                return MakeToken(TokenKind::LEQ);
            } else if (lookahead == '<') {
                GetChar();
                return MakeToken(TokenKind::LSHIFT);
            }
            return MakeToken(TokenKind::LESS);
        } else if (lookahead == ':') {
            GetChar();
            return MakeToken(TokenKind::DOUBLEDOT);
        } else if (lookahead == '/') {
            GetChar();
            return MakeToken(TokenKind::SLASH);
        } else if (lookahead == '*') {
            GetChar();
            return MakeToken(TokenKind::TIMES);
        } else if (lookahead == '`') {
            GetChar();
            return MakeToken(TokenKind::BACKTICK);
        } else if (lookahead == '$') {
            GetChar();
            return MakeToken(TokenKind::DOLLAR);
        } else if (lookahead == '.') {
            GetChar();
            return MakeToken(TokenKind::DOT);
        } else if (lookahead == '"') {
            ParseString();
            return MakeToken(TokenKind::STRING);
        } else if (lookahead == '&') {
            if (GetChar() == '&') {
                GetChar();
                return MakeToken(TokenKind::LAND);
            }
            return MakeToken(TokenKind::AND);
        } else if (lookahead == '|') {
            if (GetChar() == '|') {
                GetChar();
                return MakeToken(TokenKind::LOR);
            }
            return MakeToken(TokenKind::OR);
        } else if (lookahead == '^') {
            GetChar();
            return MakeToken(TokenKind::XOR);
        } else if (isdigit(lookahead)) {
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

