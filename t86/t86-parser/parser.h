#pragma once
#include <optional>
#include <variant>
#include <iostream>
#include <cstring>
#include <string>
#include <string_view>
#include "t86/program/programbuilder.h"
#include "t86/cpu.h"
#include "t86/program/helpers.h"
#include "common/helpers.h"
#include "fmt/core.h"
#include "common/logger.h"

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
            throw ParserError(fmt::format("{}:{}:No token beginning with '{}'",
                                          row, col, lookahead));
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
};

class Parser {
public:
    Parser(std::istream& is) : lex(is) { curtok = lex.getNext(); }

    void Section();

    tiny::t86::Register getRegister(std::string_view regname);

    tiny::t86::FloatRegister getFloatRegister(std::string_view regname);

    /// Allows only register as operand
    tiny::t86::Register Register();

    tiny::t86::FloatRegister FloatRegister();

    /// Allows only immediate as operand
    int64_t Imm();

    /// Allows f
    tiny::t86::Operand FloatImm();

    /// Allows f or F
    tiny::t86::Operand FloatImmOrRegister();

    /// Allows R or i
    tiny::t86::Operand ImmOrRegister();

    /// Allows R or R + i
    tiny::t86::Operand ImmOrRegisterOrRegisterPlusImm();

    /// Allows [i], [R], [R + i]
    tiny::t86::Operand SimpleMemory();

    /// Allows R, [i], [R], [R + i]
    tiny::t86::Operand RegisterOrSimpleMemory();

    /// Allows R, i, [i], [R], [R + i]
    tiny::t86::Operand ImmOrRegisterOrSimpleMemory();

    /// Allows R, i, R + i, [i], [R], [R + i]
    tiny::t86::Operand ImmOrRegisterPlusImmOrSimpleMemory();

    /// Allows [i], [R], [R + i], [R1 + R2], [R1 + R2 * i],
    /// [R1 + i + R2] and [R1 + i1 + R2 * i2]
    tiny::t86::Operand Memory();

    /// Parses every kind of operand excluding R + i (even floats!)
    tiny::t86::Operand Operand();

    /// Parses the operands of MOV
    std::unique_ptr<tiny::t86::MOV> ParseMOV();

    std::unique_ptr<tiny::t86::Instruction> Instruction();

    void Text();

    void Data();

    tiny::t86::Program Parse();

    template<typename ...Args>
    ParserError CreateError(fmt::format_string<Args...> format, Args&& ...args) {
        return ParserError(fmt::format("Error:{}:{}:{}", curtok.row,
                    curtok.col, fmt::format(format,
                        std::forward<Args>(args)...)));
    }
private:
    TokenKind GetNext();

    Lexer lex;
    Token curtok;
    std::vector<std::unique_ptr<tiny::t86::Instruction>> program;
    std::vector<int64_t> data;
};

