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
            throw CreateError("Registers must begin with an R, unless IP, BP or SP, got {}", regname);
        }
        regname.remove_prefix(1);
        return tiny::t86::Register{static_cast<size_t>(std::atoi(regname.data()))};
    }

    tiny::t86::FloatRegister getFloatRegister(std::string_view regname) {
        if (regname[0] != 'F') {
            throw CreateError("Float registers must begin with an F");
        }
        regname.remove_prefix(1);
        return tiny::t86::FloatRegister{static_cast<size_t>(std::atoi(regname.data()))};
    }

    /// Allows only register as operand
    tiny::t86::Register Register() {
        if (curtok.kind == TokenKind::ID) {
            std::string regname = lex.getId();
            auto reg = getRegister(regname);
            GetNext();
            return reg;
        } else {
            throw CreateError("Expected R");
        }
    }

    tiny::t86::FloatRegister FloatRegister() {
        if (curtok.kind == TokenKind::ID) {
            std::string regname = lex.getId();
            auto reg = getFloatRegister(regname);
            GetNext();
            return reg;
        } else {
            throw CreateError("Expected F");
        }
    }

    /// Allows only immediate as operand
    int64_t Imm() {
        if (curtok.kind == TokenKind::NUM) {
            auto val = lex.getNumber();
            GetNext();
            return val;
        } else {
            throw CreateError("Expected i");
        }
    }

    double FloatImm() {
        if (curtok.kind == TokenKind::FLOAT) {
            auto val = lex.getFloat();
            GetNext();
            return val;
        } else {
            throw CreateError("Expected f");
        }
    }

    tiny::t86::Operand FloatImmOrRegister() {
        if (curtok.kind == TokenKind::ID) {
            return FloatRegister();
        } else if (curtok.kind == TokenKind::FLOAT) {
            return FloatImm();
        } else {
            throw CreateError("Expected either f or F");
        }
    }

    /// Allows R or i
    tiny::t86::Operand ImmOrRegister() {
        if (curtok.kind == TokenKind::ID) {
            return Register();
        } else if (curtok.kind == TokenKind::NUM) {
            return Imm();
        } else {
            throw CreateError("Expected either i or R");
        }
    }

    /// Allows R or R + i
    tiny::t86::Operand ImmOrRegisterOrRegisterPlusImm() {
        if (curtok.kind == TokenKind::NUM) {
            return Imm();
        } else if (curtok.kind == TokenKind::ID) {
            auto reg = Register();
            if (curtok.kind == TokenKind::PLUS) {
                GetNext();
                auto imm = Imm();
                return reg + imm;
            }
            return reg;
        } else {
            throw CreateError("Expected either i, R or R + i");
        }
    }

    /// Allows [i], [R], [R + i]
    tiny::t86::Operand SimpleMemory() {
        if (curtok.kind == TokenKind::LBRACKET) {
            GetNext(); 
            if (curtok.kind == TokenKind::ID) {
                tiny::t86::Register inner = Register();
                if (curtok.kind == TokenKind::PLUS) {
                    GetNext();
                    auto imm = Imm();
                    if (curtok.kind != TokenKind::RBRACKET) {
                        throw CreateError("Expected end of ']'");
                    }
                    return tiny::t86::Mem(inner + imm);
                }
                if (curtok.kind != TokenKind::RBRACKET) {
                    throw CreateError("Expected end of ']'");
                }
                GetNext();
                return tiny::t86::Mem(inner);
            } else  {
                auto inner = Imm();
                if (curtok.kind != TokenKind::RBRACKET) {
                    throw CreateError("Expected end of ']'");
                }
                GetNext();
                return tiny::t86::Mem(inner);
            }
        } else {
            throw CreateError("Expected either [i], [R] or [R + i]");
        }
    }

    /// Allows R, [i], [R], [R + i]
    tiny::t86::Operand RegisterOrSimpleMemory() {
        if (curtok.kind == TokenKind::ID) {
            return Register();
        } else if (curtok.kind == TokenKind::LBRACKET) {
            return SimpleMemory();
        } else {
            throw CreateError("Expected either R, [i], [R] or [R + i]");
        }
    }

    /// Allows R, i, [i], [R], [R + i]
    tiny::t86::Operand ImmOrRegisterOrSimpleMemory() {
        if (curtok.kind == TokenKind::ID || curtok.kind == TokenKind::NUM) {
            return ImmOrRegister();
        } else if (curtok.kind == TokenKind::LBRACKET) {
            return SimpleMemory();
        } else {
            throw CreateError("Expected either i, R, [i], [R] or [R + i]");
        }
    }


    /// Allows R, i, R + i, [i], [R], [R + i]
    tiny::t86::Operand ImmOrRegisterPlusImmOrSimpleMemory() {
        if (curtok.kind == TokenKind::ID || curtok.kind == TokenKind::NUM) {
            return ImmOrRegisterOrRegisterPlusImm();
        } else if (curtok.kind == TokenKind::LBRACKET) {
            return SimpleMemory();
        } else {
            throw CreateError("Expected either i, R, R + i, [i], [R] or [R + i]");
        }
    }

    /// Allows [i], [R], [R + i], [R1 + R2], [R1 + R2 * i],
    /// [R1 + i + R2] and [R1 + i1 + R2 * i2]
    tiny::t86::Operand Memory() {
        using namespace tiny::t86;
        std::optional<tiny::t86::Operand> result;
        if (curtok.kind == TokenKind::LBRACKET) {
            GetNext();
            // Must be [i]
            if (curtok.kind == TokenKind::NUM) {
                auto imm = lex.getNumber();
                GetNext();
                result = Mem(imm);
            } else if (curtok.kind == TokenKind::ID) {
                auto regname1 = lex.getId();
                auto reg1 = getRegister(regname1);
                GetNext();
                if (curtok.kind == TokenKind::PLUS) {
                    GetNext();
                    // Can be [R + i], [R + i + R] or [R + i + R * i]
                    if (curtok.kind == TokenKind::NUM) {
                        auto imm1 = lex.getNumber();
                        GetNext();
                        // Can be either [R + i + R] or [R + i + R * i]
                        if (curtok.kind == TokenKind::PLUS) {
                            GetNext();
                            if (curtok.kind == TokenKind::ID) {
                                auto regname2 = lex.getId();
                                auto reg2 = getRegister(regname2);
                                GetNext();
                                // Must be [R + i + R * i]
                                if (curtok.kind == TokenKind::TIMES) {
                                    GetNext();
                                    auto imm2 = lex.getNumber();
                                    GetNext();
                                    result = Mem(reg1 + imm1 + reg2 * imm2);
                                // Must be [R + i + R]
                                } else {
                                    result = Mem(reg1 + imm1 + reg2);
                                }
                            } else {
                                throw CreateError("Expected R");
                            }
                        // Must be [R + i]
                        } else {
                            result = Mem(reg1 + imm1);
                        }
                    } else if (curtok.kind == TokenKind::ID) {
                        auto regname2 = lex.getId();
                        auto reg2 = getRegister(regname2);
                        GetNext();
                        // Must be [R + R + i]
                        if (curtok.kind == TokenKind::TIMES) {
                            GetNext();
                            if (curtok.kind == TokenKind::NUM) {
                                auto imm = lex.getNumber();
                                GetNext();
                                result = Mem(reg1 + reg2 * imm);
                            }
                        // Must be [R + R]
                        } else {
                            result = Mem(reg1 + reg2);
                        }
                    } else {
                        throw CreateError("Expected either i or R");
                    }
                // Must be [R * i]
                } else if (curtok.kind == TokenKind::TIMES) {
                    GetNext();
                    if (curtok.kind == TokenKind::NUM) {
                        auto imm = lex.getNumber();
                        GetNext();
                        result = Mem(reg1 * imm);
                    } else {
                        throw CreateError("Expected 'i'");
                    }
                // Must be [R]
                } else {
                    GetNext();
                    result = Mem(reg1);
                }
            } else {
                throw CreateError("Expected either R or i");
            }
        } else {
            throw CreateError("Expected '['");
        }
        if (curtok.kind != TokenKind::RBRACKET) {
            throw CreateError("Expected ']'");
        }
        GetNext();
        assert(result);
        return *result;
    }

    /// Parses every kind of operand, specially for MOV
    tiny::t86::Operand Operand() {
        using namespace tiny::t86;
        if (curtok.kind == TokenKind::ID) {
            std::string regname = lex.getId();
            GetNext();
            // Reg + Imm
            if (curtok.kind == TokenKind::PLUS) {
                if (GetNext() != TokenKind::NUM) {
                    throw CreateError("After Reg + _ there can be only number");
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

#define PARSE_BINARY(TYPE, DEST_PARSE, FROM_PARSE) \
    if (ins_name == #TYPE) { \
        auto dest = DEST_PARSE(); \
        if (curtok.kind != TokenKind::COMMA) { \
            throw CreateError("Expected ','"); \
        } \
        GetNext(); \
        auto from = FROM_PARSE(); \
        return std::make_unique<tiny::t86::TYPE>(std::move(dest), std::move(from)); \
    }

#define PARSE_UNARY(TYPE, OPERAND_PARSE) \
    if (ins_name == #TYPE) { \
        auto op = OPERAND_PARSE(); \
        return std::make_unique<tiny::t86::TYPE>(std::move(op)); \
    }

#define PARSE_NULLARY(TYPE) \
    if (ins_name == #TYPE) { \
        return std::make_unique<tiny::t86::TYPE>(); \
    }

    std::unique_ptr<tiny::t86::Instruction> Instruction() {
        // Address at the beginning is optional
        if (curtok.kind == TokenKind::NUM) {
            GetNextPrev();
        }

        ExpectTok(TokenKind::ID, curtok.kind, []{ return "Expected register name"; });
        std::string ins_name = lex.getId();
        GetNextPrev();

        // MOV is a special snowflake in a sense that it allows
        // very big range of operands, but they have very restrictive
        // relationships. So we just allow everything and hope
        // it won't explode.
        if (ins_name == "MOV") {
            auto dest = Operand();
            if (curtok.kind != TokenKind::COMMA) {
                throw CreateError("Expected ','");
            }
            GetNext();
            auto from = Operand();
            return std::make_unique<tiny::t86::MOV>(dest, from);
        }

        if (ins_name == "LEA") {
            auto dest = Register();
            if (curtok.kind != TokenKind::COMMA) {
                throw CreateError("Expected ','");
            }
            GetNext();
            auto from = Memory();
            // Memory allows [R], which LEA doesn't like.
            if (from.isMemoryRegister() || from.isMemoryImmediate()) {
                throw CreateError("LEA doesn't support [R] or [i]");
            }
            return std::make_unique<tiny::t86::LEA>(dest, from);
        }

        PARSE_BINARY(ADD, Register, ImmOrRegisterPlusImmOrSimpleMemory);
        PARSE_BINARY(SUB, Register, ImmOrRegisterPlusImmOrSimpleMemory);
        PARSE_BINARY(MUL, Register, ImmOrRegisterPlusImmOrSimpleMemory);
        PARSE_BINARY(DIV, Register, ImmOrRegisterPlusImmOrSimpleMemory);
        PARSE_BINARY(IMUL, Register, ImmOrRegisterPlusImmOrSimpleMemory);
        PARSE_BINARY(IDIV, Register, ImmOrRegisterPlusImmOrSimpleMemory);
        PARSE_BINARY(AND, Register, ImmOrRegisterPlusImmOrSimpleMemory);
        PARSE_BINARY(OR, Register, ImmOrRegisterPlusImmOrSimpleMemory);
        PARSE_BINARY(XOR, Register, ImmOrRegisterPlusImmOrSimpleMemory);
        PARSE_BINARY(LSH, Register, ImmOrRegisterPlusImmOrSimpleMemory);
        PARSE_BINARY(RSH, Register, ImmOrRegisterPlusImmOrSimpleMemory);
        PARSE_BINARY(CMP, Register, ImmOrRegisterOrSimpleMemory);
        PARSE_BINARY(LOOP, Register, ImmOrRegister);
        PARSE_BINARY(FADD, FloatRegister, FloatImmOrRegister);
        PARSE_BINARY(FSUB, FloatRegister, FloatImmOrRegister);
        PARSE_BINARY(FMUL, FloatRegister, FloatImmOrRegister);
        PARSE_BINARY(FDIV, FloatRegister, FloatImmOrRegister);
        PARSE_BINARY(FCMP, FloatRegister, FloatImmOrRegister);
        // FIXME: The second 'Operand' is wrong, as it only allows
        // everything operand does but only memory accesses, ie.
        // [R1 + 1 + R2 * 3] is fine but R1 + 1 is not.
        PARSE_BINARY(LEA, Register, Operand);
        PARSE_BINARY(EXT, FloatRegister, Register);
        PARSE_BINARY(NRW, Register, FloatRegister);

        PARSE_UNARY(INC, Register);
        PARSE_UNARY(DEC, Register);
        PARSE_UNARY(NEG, Register);
        PARSE_UNARY(NOT, Register);
        PARSE_UNARY(JMP, ImmOrRegister);
        PARSE_UNARY(JZ, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JNZ, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JE, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JNE, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JG, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JGE, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JL, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JLE, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JA, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JAE, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JB, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JBE, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JO, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JNO, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JS, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(JNS, ImmOrRegisterOrSimpleMemory);
        PARSE_UNARY(CALL, ImmOrRegister);
        PARSE_UNARY(PUSH, ImmOrRegister);
        PARSE_UNARY(PUSH, FloatImmOrRegister);
        PARSE_UNARY(POP, Register);
        PARSE_UNARY(FPOP, FloatRegister);
        PARSE_UNARY(PUTCHAR, Register);
        PARSE_UNARY(PUTNUM, Register);
        PARSE_UNARY(GETCHAR, Register);

        PARSE_NULLARY(HALT);
        PARSE_NULLARY(NOP);
        PARSE_NULLARY(BKPT);
        PARSE_NULLARY(BREAK);
        PARSE_NULLARY(RET);

        if (ins_name == "DBG") {
            // TODO: This probably won't be used anymore. It would be very
            // difficult (impossible) to to pass lambda in text file
            throw ParserError("DBG instruction is not supported");
        }

        throw ParserError(fmt::format("Unknown instruction {}", ins_name));
    }

#undef PARSE_BINARY
#undef PARSE_UNARY
#undef PARSE_NULLARY

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
            throw ParserError("File does not contain any sections");
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

