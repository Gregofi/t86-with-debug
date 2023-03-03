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

    void ParseNumber() {
        int neg = lookahead == '-' ? -1 : 1;
        if (neg == -1) {
            lookahead = GetChar();
        }
        std::string num{lookahead};
        while (true) {
            lookahead = GetChar();
            if (!isdigit(lookahead)) {
                break;
            }
            num += lookahead;
        }
        number = neg * std::stoi(num);
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
            while(lookahead != EOF && lookahead != '\n') {
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
            ParseNumber();
            return MakeToken(TokenKind::NUM);
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
    std::string id;
    std::string str;

    std::istream& input;
    char lookahead;
};

class Parser {
public:
    Parser(std::istream& is) : lex(is) { curtok = lex.getNext(); }
    static void ExpectTok(TokenKind expected, TokenKind tok, std::function<std::string()> message) {
        if (expected != tok) {
            throw ParserError(message());
        }
    }

    /// Fetches new token and returns previous (the one that was in curtok.kind until this function call) one.
    TokenKind GetNextPrev() {
        auto prev = curtok;
        curtok = lex.getNext();
        return prev.kind;
    }

    TokenKind GetNext() {
        curtok = lex.getNext();
        return curtok.kind;
    }

    void Section() {
        ExpectTok(TokenKind::ID, curtok.kind, []{ return "Expected '.section_name'"; });
        std::string section_name = lex.getId();
        GetNext();

        log_info("Parsing '{}' section", section_name);
        if (section_name == "text") {
            Text();
        } else if (section_name == "data") {
            Data();
        } else {
            log_info("Skipping '{}' section", section_name);
            while (curtok.kind != TokenKind::DOT && curtok.kind != TokenKind::END) {
                GetNext();
            };
        }
    }

    tiny::t86::Register getRegister(std::string_view regname) {
        if (regname == "BP") {
            return tiny::t86::Register::StackBasePointer();
        } else if (regname == "SP") {
            return tiny::t86::Register::StackPointer();
        } else if (regname == "IP") {
            return tiny::t86::Register::ProgramCounter();
        } else if (regname[0] != 'R') {
            throw ParserError(fmt::format("Registers must begin with an R, unless IP, BP or SP, got {}", regname));
        }
        regname.remove_prefix(1);
        return tiny::t86::Register{static_cast<size_t>(std::atoi(regname.data()))};
    }

    tiny::t86::Operand Operand() {
        using namespace tiny::t86;
        if (curtok.kind == TokenKind::ID) {
            std::string regname = lex.getId();
            GetNext();
            // Reg + Imm
            if (curtok.kind == TokenKind::PLUS) {
                if (GetNext() != TokenKind::NUM) {
                    throw ParserError("After Reg + _ there can be only number");
                }
                int imm = lex.getNumber();
                GetNext();
                return getRegister(regname) + imm;
            }
            auto reg = getRegister(regname);
            return reg;
        } else if (curtok.kind == TokenKind::NUM) {
            int imm = lex.getNumber();
            GetNext();
            return imm;
        } else if (curtok.kind == TokenKind::LBRACKET) {
            // First is imm
            if (GetNext() == TokenKind::NUM) {
                auto val = lex.getNumber();
                GetNext(); // 'num'
                GetNext(); // ']'
                // [i]
                return Mem(static_cast<uint64_t>(val));
            // First is register
            } else if (curtok.kind == TokenKind::ID) {
                auto regname = lex.getId();
                auto reg = getRegister(regname);
                // Only register
                if (GetNext() == TokenKind::RBRACKET) {
                    GetNext();
                    // [Rx]
                    return Mem(reg);
                }
                // Has second with +
                if (curtok.kind == TokenKind::PLUS) {
                    GetNext();
                    // Can either be imm or register
                    if (curtok.kind == TokenKind::ID) {
                        auto reg2 = getRegister(lex.getId());
                        if (GetNext() == TokenKind::RBRACKET) {
                            GetNext();
                            return Mem(reg + reg2);
                        } else if (curtok.kind == TokenKind::TIMES && GetNext() == TokenKind::NUM) {
                            auto val = lex.getNumber();
                            ExpectTok(TokenKind::RBRACKET, GetNext(), []{ return "Expected ']' to close dereference\n"; });
                            return Mem(reg + reg2 * val);
                        }
                    } else if (curtok.kind == TokenKind::NUM) {
                        auto val = lex.getNumber();
                        if (GetNext() == TokenKind::RBRACKET) {
                            GetNext();
                            return Mem(reg + val);
                        }
                        if (curtok.kind != TokenKind::PLUS || GetNext() != TokenKind::ID) {
                            throw ParserError("Dereference of form [R1 + i ...] must always contain `+ R` after i");
                        }

                        auto reg2 = getRegister(lex.getId());
                        if (GetNext() == TokenKind::RBRACKET) {
                            GetNext();
                            return Mem(reg + val + reg2);
                        }
                        ExpectTok(TokenKind::TIMES, curtok.kind, []{ return "After `[R1 + i + R2` there must always be a `*` or `]`"; });
                        ExpectTok(TokenKind::NUM, GetNext(), []{ return "After `[R1 + i + R2 *` there must always be an imm"; });
                        auto val2 = lex.getNumber();
                        
                        ExpectTok(TokenKind::RBRACKET, GetNext(), []{ return "Expected ']' to close dereference "; });
                        GetNext(); // Eat ']'
                        return Mem(reg + val + reg2 * val2);
                    }
                // Has second with *
                } else if (curtok.kind == TokenKind::TIMES) {
                    // There must be imm now
                    ExpectTok(TokenKind::NUM, GetNext(), []{ return "After [R1 * ...] there must always be an imm"; });
                    int val = lex.getNumber();
                    if (GetNext() != TokenKind::RBRACKET) {
                        throw ParserError("Expected ']' to close dereference");
                    }
                    GetNext(); // ']'
                    return Mem(reg * val);
                }
                UNREACHABLE;
            }
            NOT_IMPLEMENTED;
        }
        UNREACHABLE;
    }

#define CHECK_COMMA() do { ExpectTok(TokenKind::COMMA, GetNextPrev(), []{ return "Expected comma to separate arguments"; });} while (false)

    std::unique_ptr<tiny::t86::Instruction> Instruction() {
        // Address at the beginning is optional
        if (curtok.kind == TokenKind::NUM) {
            GetNextPrev();
        }

        ExpectTok(TokenKind::ID, curtok.kind, []{ return "Expected register name"; });
        std::string ins_name = lex.getId();
        GetNextPrev();

        if (ins_name == "MOV") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::MOV>(dest, from);
        } else if (ins_name == "ADD") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::ADD>(dest.getRegister(), from);
        } else if (ins_name == "LEA") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::LEA>(dest.getRegister(), from);
        } else if (ins_name == "HALT") {
            return std::make_unique<tiny::t86::HALT>();
        } else if (ins_name == "DBG") {
            // TODO: This probably won't be used anymore. It would be very difficult (impossible) to
            //       to pass lambda in text file
            throw ParserError("DBG instruction is not supported");
        } else if (ins_name == "BKPT") {
            return std::make_unique<tiny::t86::BKPT>();
        } else if (ins_name == "BREAK") {
            return std::make_unique<tiny::t86::BREAK>();
        } else if (ins_name == "SUB") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::SUB>(dest.getRegister(), from);
        } else if (ins_name == "INC") {
            auto op = Operand();
            return std::make_unique<tiny::t86::INC>(op.getRegister());
        } else if (ins_name == "DEC") {
            auto op = Operand();
            return std::make_unique<tiny::t86::DEC>(op.getRegister());
        } else if (ins_name == "NEG") {
            auto op = Operand();
            return std::make_unique<tiny::t86::DEC>(op.getRegister());
        } else if (ins_name == "MUL") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::MUL>(dest.getRegister(), from);
        } else if (ins_name == "DIV") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::DIV>(dest.getRegister(), from);
        } else if (ins_name == "MOD") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::MOD>(dest.getRegister(), from);
        } else if (ins_name == "IMUL") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::IMUL>(dest.getRegister(), from);
        } else if (ins_name == "IDIV") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::IDIV>(dest.getRegister(), from);
        } else if (ins_name == "AND") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::AND>(dest.getRegister(), from);
        } else if (ins_name == "OR") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::OR>(dest.getRegister(), from);
        } else if (ins_name == "XOR") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::XOR>(dest.getRegister(), from);
        } else if (ins_name == "NOT") {
            auto op = Operand();
            return std::make_unique<tiny::t86::NOT>(op.getRegister());
        } else if (ins_name == "LSH") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::LSH>(dest.getRegister(), from);
        } else if (ins_name == "RSH") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::RSH>(dest.getRegister(), from);
        } else if (ins_name == "CLF") {
            throw ParserError("CLF instruction is not implemented");
        } else if (ins_name == "CMP") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return std::make_unique<tiny::t86::CMP>(dest.getRegister(), from);
        } else if (ins_name == "FCMP") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            if (from.isFloatValue()) {
                NOT_IMPLEMENTED;
                // return std::make_unique<tiny::t86::FCMP>(, from.getFloatValue());
            } else if (from.isFloatRegister()) {
                return std::make_unique<tiny::t86::FCMP>(dest.getFloatRegister(), from.getFloatRegister());
            } else {
                throw ParserError("FCMP must have either float value or float register as dest");
            }
        } else if (ins_name == "JMP") {
            auto dest = Operand();
            if (dest.isRegister()) {
                return std::make_unique<tiny::t86::JMP>(dest.getRegister());
            } else if (dest.isValue()) {
                return std::make_unique<tiny::t86::JMP>(static_cast<uint64_t>(dest.getValue()));
            } else {
                throw ParserError("JMP must have either register or value as dest");
            }
        } else if (ins_name == "LOOP") {
            auto reg = Operand();
            CHECK_COMMA();
            auto address = Operand();
            if (address.isRegister()) {
                return std::make_unique<tiny::t86::LOOP>(reg.getRegister(), address.getRegister());
            } else if (address.isValue()) {
                return std::make_unique<tiny::t86::LOOP>(reg.getRegister(), static_cast<uint64_t>(address.getValue()));
            } else {
                throw ParserError("LOOP must have either register or value as dest");
            }
        } else if (ins_name == "JZ") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JZ>(dest);
        } else if (ins_name == "JNZ") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JNZ>(dest);
        } else if (ins_name == "JE") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JE>(dest);
        } else if (ins_name == "JNE") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JNE>(dest);
        } else if (ins_name == "JG") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JG>(dest);
        } else if (ins_name == "JGE") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JGE>(dest);
        } else if (ins_name == "JL") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JL>(dest);
        } else if (ins_name == "JLE") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JLE>(dest);
        } else if (ins_name == "JA") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JA>(dest);
        } else if (ins_name == "JAE") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JAE>(dest);
        } else if (ins_name == "JB") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JB>(dest);
        } else if (ins_name == "JBE") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JBE>(dest);
        } else if (ins_name == "JO") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JO>(dest);
        } else if (ins_name == "JNO") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JNO>(dest);
        } else if (ins_name == "JS") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JS>(dest);
        } else if (ins_name == "JNS") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::JNS>(dest);
        } else if (ins_name == "CALL") {
            auto dest = Operand();
            return std::make_unique<tiny::t86::CALL>(dest);
        } else if (ins_name == "RET") {
            return std::make_unique<tiny::t86::RET>();
        } else if (ins_name == "PUSH") {
            auto val = Operand();
            return std::make_unique<tiny::t86::PUSH>(val);
        } else if (ins_name == "FPUSH") {
            auto val = Operand();
            return std::make_unique<tiny::t86::FPUSH>(val);
        } else if (ins_name == "POP") {
            auto reg = Operand();
            return std::make_unique<tiny::t86::POP>(reg.getRegister());
        } else if (ins_name == "FPOP") {
            auto reg = Operand();
            return std::make_unique<tiny::t86::FPOP>(reg.getFloatRegister());
        } else if (ins_name == "PUTCHAR") {
            auto reg = Operand();
            return std::make_unique<tiny::t86::PUTCHAR>(reg.getRegister(), std::cout);
        } else if (ins_name == "PUTNUM") {
            auto reg = Operand();
            return std::make_unique<tiny::t86::PUTNUM>(reg.getRegister(), std::cout);
        } else if (ins_name == "FADD") {
            NOT_IMPLEMENTED;
        } else if (ins_name == "FSUB") {
            NOT_IMPLEMENTED;
        } else if (ins_name == "FMUL") {
            NOT_IMPLEMENTED;
        } else if (ins_name == "FDIV") {
            NOT_IMPLEMENTED;
        } else if (ins_name == "EXT") {
            NOT_IMPLEMENTED;
        } else if (ins_name == "NRW") {
            NOT_IMPLEMENTED;
        } else if (ins_name == "NOP") {
            return std::make_unique<tiny::t86::NOP>();
        } else {
            throw ParserError(fmt::format("Unknown instruction {}", ins_name));
        }
    }

#undef CHECK_COMMA

    void Text() {
        while (curtok.kind == TokenKind::NUM || curtok.kind == TokenKind::ID) {
            auto ins = Instruction();
            program.push_back(std::move(ins));
            // if (GetNextPrev() != TokenKind::SEMICOLON) {
            //     throw ParserError("Instruction must be terminated by semicolon");
            // }
        }
    }

    void Data() {
        while (curtok.kind == TokenKind::STRING || curtok.kind == TokenKind::NUM) {
            if (curtok.kind == TokenKind::STRING) {
                auto string = lex.getStr();
                std::transform(string.begin(), string.end(),
                               std::back_inserter(data),
                               [](auto &&c) { return c; });
            } else if (curtok.kind == TokenKind::NUM) {
                data.emplace_back(lex.getNumber());
            }
            GetNext();
        }
    }

    tiny::t86::Program Parse() {
        if (curtok.kind != TokenKind::DOT) {
            throw ParserError("File does not contain any section");
        }
        while (GetNextPrev() == TokenKind::DOT) {
            Section();
        }
        return {std::move(program), std::move(data)};
    }

    template<typename ...Args>
    ParserError CreateError(fmt::format_string<Args...> format, Args&& ...args) {
        return ParserError(fmt::format("Error:{}:{}:{}", curtok.row,
                    curtok.col, fmt::format(format,
                        std::forward<Args>(args)...)));
    }
private:
    Lexer lex;
    Token curtok;
    std::vector<std::unique_ptr<tiny::t86::Instruction>> program;
    std::vector<int64_t> data;
};

