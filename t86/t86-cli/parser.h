#pragma once
#include <optional>
#include <variant>
#include <iostream>
#include <cstring>
#include <string>
#include <string_view>
#include "../t86/program/programbuilder.h"
#include "../t86/cpu.h"
#include "../t86/program/helpers.h"
#include "../common/helpers.h"


enum class Token {
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
};

/// Parses text representation of T86 into in-memory representation
class Lexer {
public:
    explicit Lexer(std::istream& input) noexcept : input(input) { }

    Token getNext() {
        char c = input.get();
        
        if (c == '#') {
            while(c != EOF && c != '\n') {
                c = input.get();
            }
            return getNext();
        } else if (c == EOF) {
            return Token::END;
        } else if (isspace(c)) {
            return getNext();
        } else if (c == ';') {
            return Token::SEMICOLON;
        } else if (c == ',') {
            return Token::COMMA;
        } else if (c == '[') {
            return Token::LBRACKET;
        } else if (c == ']') {
            return Token::RBRACKET;
        } else if (c == '+') {
            return Token::PLUS;
        } else if (c == '*') {
            return Token::TIMES;
        } else if (c == '.') {
            return Token::DOT;
        } else if (isdigit(c) || c == '-') {
            int neg = c == '-' ? -1 : 1;
            if (neg == -1) {
                c = input.get();
            }
            std::string num{c};
            while (true) {
                c = input.get();
                if (!isdigit(c)) {
                    input.unget();
                    break;
                }
                num += c;
            }
            number = neg * std::stoi(num);
            return Token::NUM;
        } else {
            std::string str{c};
            while (true) {
                c = input.get();
                if (!isalnum(c)) {
                    input.unget();
                    break;
                }
                str += c;
            }
            id = str;
            return Token::ID;
        }
    }

    std::string getId() const { return id; }
    int getNumber() const noexcept { return number; }
private:
    int number{-1};
    std::string id;
    std::istream& input;
};

class ParserError : std::exception {
public:
    ParserError(std::string message) : message(std::move(message)) { }
    const char* what() const noexcept override {
        return message.c_str();
    }
private:
    std::string message;
};

class Parser {
public:
    Parser(std::istream& is) : lex(is) { curtok = lex.getNext(); }
    static void ExpectTok(Token expected, Token tok, std::function<std::string()> message) {
        if (expected != tok) {
            throw ParserError(message());
        }
    }

    /// Fetches new token and returns previous (the one that was in curtok until this function call) one.
    Token GetNextPrev() {
        auto prev = curtok;
        curtok = lex.getNext();
        return prev;
    }

    Token GetNext() {
        return curtok = lex.getNext();
    }

    void Section() {
        ExpectTok(Token::ID, curtok, []{ return "Expected '.section_name'"; });
        std::string section_name = lex.getId();
        GetNextPrev();
        if (section_name == "text") {
            Text();
        } else {
            throw ParserError("Invalid section name");
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
            throw ParserError(utils::format("Registers must begin with an R, unless IP, BP or SP, got {}", regname));
        }
        regname.remove_prefix(1);
        return tiny::t86::Register{static_cast<size_t>(std::atoi(regname.data()))};
    }

    tiny::t86::Operand Operand() {
        using namespace tiny::t86;
        if (curtok == Token::ID) {
            std::string regname = lex.getId();
            GetNext();
            // Reg + Imm
            if (curtok == Token::PLUS) {
                if (GetNext() != Token::NUM) {
                    throw ParserError("After Reg + _ there can be only number");
                }
                int imm = lex.getNumber();
                GetNext();
                return getRegister(regname) + imm;
            }
            auto reg = getRegister(regname);
            return reg;
        } else if (curtok == Token::NUM) {
            int imm = lex.getNumber();
            GetNext();
            return imm;
        } else if (curtok == Token::LBRACKET) {
            // First is imm
            if (GetNext() == Token::NUM) {
                auto val = lex.getNumber();
                GetNext(); // 'num'
                GetNext(); // ']'
                // [i]
                return Mem(static_cast<uint64_t>(val));
            // First is register
            } else if (curtok == Token::ID) {
                auto regname = lex.getId();
                auto reg = getRegister(regname);
                // Only register
                if (GetNext() == Token::RBRACKET) {
                    GetNext();
                    // [Rx]
                    return Mem(reg);
                }
                // Has second with +
                if (curtok == Token::PLUS) {
                    GetNext();
                    // Can either be imm or register
                    if (curtok == Token::ID) {
                        auto reg2 = getRegister(lex.getId());
                        if (GetNext() == Token::RBRACKET) {
                            GetNext();
                            return Mem(reg + reg2);
                        } else if (curtok == Token::TIMES && GetNext() == Token::NUM) {
                            auto val = lex.getNumber();
                            ExpectTok(Token::RBRACKET, GetNext(), []{ return "Expected ']' to close dereference\n"; });
                            return Mem(reg + reg2 * val);
                        }
                    } else if (curtok == Token::NUM) {
                        auto val = lex.getNumber();
                        if (GetNext() == Token::RBRACKET) {
                            GetNext();
                            return Mem(reg + val);
                        }
                        if (curtok != Token::PLUS || GetNext() != Token::ID) {
                            throw ParserError("Dereference of form [R1 + i ...] must always contain `+ R` after i");
                        }

                        auto reg2 = getRegister(lex.getId());
                        if (GetNext() == Token::RBRACKET) {
                            GetNext();
                            return Mem(reg + val + reg2);
                        }
                        ExpectTok(Token::TIMES, curtok, []{ return "After `[R1 + i + R2` there must always be a `*` or `]`"; });
                        ExpectTok(Token::NUM, GetNext(), []{ return "After `[R1 + i + R2 *` there must always be an imm"; });
                        auto val2 = lex.getNumber();
                        
                        ExpectTok(Token::RBRACKET, GetNext(), []{ return "Expected ']' to close dereference "; });
                        GetNext(); // Eat ']'
                        return Mem(reg + val + reg2 * val2);
                    }
                // Has second with *
                } else if (curtok == Token::TIMES) {
                    // There must be imm now
                    ExpectTok(Token::NUM, GetNext(), []{ return "After [R1 * ...] there must always be an imm"; });
                    int val = lex.getNumber();
                    if (GetNext() != Token::RBRACKET) {
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

#define CHECK_COMMA() do { ExpectTok(Token::COMMA, GetNextPrev(), []{ return "Expected comma to separate arguments"; });} while (false)

    tiny::t86::Instruction* Instruction() {
        // Address at the beginning is optional
        if (curtok == Token::NUM) {
            GetNextPrev();
        }

        ExpectTok(Token::ID, curtok, []{ return "Expected register name"; });
        std::string ins_name = lex.getId();
        GetNextPrev();

        if (ins_name == "MOV") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return new tiny::t86::MOV{dest, from};
        } else if (ins_name == "ADD") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();

            return new tiny::t86::ADD{dest.getRegister(), from};
        } else if (ins_name == "LEA") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();

            return new tiny::t86::LEA(dest.getRegister(), from);
        } else if (ins_name == "HALT") {
            return new tiny::t86::HALT{};
        } else if (ins_name == "DBG") {
            // TODO: This probably won't be used anymore. It would be very difficult (impossible) to
            //       to pass lambda in text file
            throw ParserError("DBG instruction is not supported");
        } else if (ins_name == "BREAK") {
            return new tiny::t86::BREAK{};
        } else if (ins_name == "SUB") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return new tiny::t86::SUB{dest.getRegister(), from};
        } else if (ins_name == "INC") {
            auto op = Operand();
            return new tiny::t86::INC{op.getRegister()};
        } else if (ins_name == "DEC") {
            auto op = Operand();
            return new tiny::t86::DEC{op.getRegister()};
        } else if (ins_name == "NEG") {
            auto op = Operand();
            return new tiny::t86::DEC{op.getRegister()};
        } else if (ins_name == "MUL") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return new tiny::t86::MUL{dest.getRegister(), from};
        } else if (ins_name == "DIV") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return new tiny::t86::DIV{dest.getRegister(), from};
        } else if (ins_name == "MOD") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return new tiny::t86::MOD{dest.getRegister(), from};
        } else if (ins_name == "IMUL") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return new tiny::t86::IMUL{dest.getRegister(), from};
        } else if (ins_name == "IDIV") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return new tiny::t86::IDIV{dest.getRegister(), from};
        } else if (ins_name == "AND") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return new tiny::t86::AND{dest.getRegister(), from};
        } else if (ins_name == "OR") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return new tiny::t86::OR{dest.getRegister(), from};
        } else if (ins_name == "XOR") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return new tiny::t86::XOR{dest.getRegister(), from};
        } else if (ins_name == "NOT") {
            auto op = Operand();
            return new tiny::t86::NOT{op.getRegister()};
        } else if (ins_name == "LSH") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return new tiny::t86::LSH{dest.getRegister(), from};
        } else if (ins_name == "RSH") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return new tiny::t86::RSH{dest.getRegister(), from};
        } else if (ins_name == "CLF") {
            throw ParserError("CLF instruction is not implemented");
        } else if (ins_name == "CMP") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            return new tiny::t86::CMP{dest.getRegister(), from};
        } else if (ins_name == "FCMP") {
            auto dest = Operand();
            CHECK_COMMA();
            auto from = Operand();
            if (from.isFloatValue()) {
                NOT_IMPLEMENTED;
                // return new tiny::t86::FCMP{, from.getFloatValue()};
            } else if (from.isFloatRegister()) {
                return new tiny::t86::FCMP{dest.getFloatRegister(), from.getFloatRegister()};
            } else {
                throw ParserError("FCMP must have either float value or float register as dest");
            }
        } else if (ins_name == "JMP") {
            auto dest = Operand();
            if (dest.isRegister()) {
                return new tiny::t86::JMP{dest.getRegister()};
            } else if (dest.isValue()) {
                return new tiny::t86::JMP{static_cast<uint64_t>(dest.getValue())};
            } else {
                throw ParserError("JMP must have either register or value as dest");
            }
        } else if (ins_name == "LOOP") {
            auto reg = Operand();
            CHECK_COMMA();
            auto address = Operand();
            if (address.isRegister()) {
                return new tiny::t86::LOOP{reg.getRegister(), address.getRegister()};
            } else if (address.isValue()) {
                return new tiny::t86::LOOP{reg.getRegister(), static_cast<uint64_t>(address.getValue())};
            } else {
                throw ParserError("LOOP must have either register or value as dest");
            }
        } else if (ins_name == "JZ") {
            auto dest = Operand();
            return new tiny::t86::JZ(dest);
        } else if (ins_name == "JNZ") {
            auto dest = Operand();
            return new tiny::t86::JNZ(dest);
        } else if (ins_name == "JE") {
            auto dest = Operand();
            return new tiny::t86::JE(dest);
        } else if (ins_name == "JNE") {
            auto dest = Operand();
            return new tiny::t86::JNE(dest);
        } else if (ins_name == "JG") {
            auto dest = Operand();
            return new tiny::t86::JG(dest);
        } else if (ins_name == "JGE") {
            auto dest = Operand();
            return new tiny::t86::JGE(dest);
        } else if (ins_name == "JL") {
            auto dest = Operand();
            return new tiny::t86::JL(dest);
        } else if (ins_name == "JLE") {
            auto dest = Operand();
            return new tiny::t86::JLE(dest);
        } else if (ins_name == "JA") {
            auto dest = Operand();
            return new tiny::t86::JA(dest);
        } else if (ins_name == "JAE") {
            auto dest = Operand();
            return new tiny::t86::JAE(dest);
        } else if (ins_name == "JB") {
            auto dest = Operand();
            return new tiny::t86::JB(dest);
        } else if (ins_name == "JBE") {
            auto dest = Operand();
            return new tiny::t86::JBE(dest);
        } else if (ins_name == "JO") {
            auto dest = Operand();
            return new tiny::t86::JO(dest);
        } else if (ins_name == "JNO") {
            auto dest = Operand();
            return new tiny::t86::JNO(dest);
        } else if (ins_name == "JS") {
            auto dest = Operand();
            return new tiny::t86::JS(dest);
        } else if (ins_name == "JNS") {
            auto dest = Operand();
            return new tiny::t86::JNS(dest);
        } else if (ins_name == "CALL") {
            auto dest = Operand();
            return new tiny::t86::CALL(dest);
        } else if (ins_name == "RET") {
            return new tiny::t86::RET();
        } else if (ins_name == "PUSH") {
            auto val = Operand();
            return new tiny::t86::PUSH{val};
        } else if (ins_name == "FPUSH") {
            auto val = Operand();
            return new tiny::t86::FPUSH{val};
        } else if (ins_name == "POP") {
            auto reg = Operand();
            return new tiny::t86::POP{reg.getRegister()};
        } else if (ins_name == "FPOP") {
            auto reg = Operand();
            return new tiny::t86::FPOP{reg.getFloatRegister()};
        } else if (ins_name == "PUTCHAR") {
            auto reg = Operand();
            return new tiny::t86::PUTCHAR{reg.getRegister(), std::cout};
        } else if (ins_name == "PUTNUM") {
            auto reg = Operand();
            return new tiny::t86::PUTNUM{reg.getRegister(), std::cout};
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
            return new tiny::t86::NOP{};
        } else {
            throw ParserError(utils::format("Unknown instruction {}", ins_name));
        }
    }

#undef CHECK_COMMA

    void Text() {
        utils::logger("Parsing text section...");
        while (curtok == Token::NUM || curtok == Token::ID) {
            auto ins = Instruction();
            program.push_back(ins);
            // if (GetNextPrev() != Token::SEMICOLON) {
            //     throw ParserError("Instruction must be terminated by semicolon");
            // }
        }
    }

    tiny::t86::Program Parse() {
        if (curtok != Token::DOT) {
            throw ParserError("File does not contain any section");
        }
        while (GetNextPrev() == Token::DOT) {
            Section();
        }
        return program;
    }
private:
    Lexer lex;
    Token curtok;
    std::vector<tiny::t86::Instruction*> program;
    tiny::t86::ProgramBuilder builder;
};

