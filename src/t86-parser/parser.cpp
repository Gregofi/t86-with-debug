#include "parser.h"


TokenKind Parser::GetNext() {
    curtok = lex.getNext();
    return curtok.kind;
}

void Parser::Section() {
    if (curtok.kind != TokenKind::ID) {
        throw CreateError("Expected '.section_name'");
    }
    std::string section_name = lex.getId();
    GetNext();

    log_info("Parsing '{}' section", section_name);
    if (section_name == "text") {
        Text();
    } else if (section_name == "data") {
        Data();
    } else {
        log_info("Skipping '{}' section", section_name);
        lex.SetIgnoreMode(true);
        while (curtok.kind != TokenKind::DOT && curtok.kind != TokenKind::END) {
            GetNext();
        };
        lex.SetIgnoreMode(false);
    }
}

tiny::t86::Register Parser::getRegister(std::string_view regname) {
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

tiny::t86::FloatRegister Parser::getFloatRegister(std::string_view regname) {
    if (regname[0] != 'F') {
        throw CreateError("Float registers must begin with an F");
    }
    regname.remove_prefix(1);
    return tiny::t86::FloatRegister{static_cast<size_t>(std::atoi(regname.data()))};
}

/// Allows only register as operand
tiny::t86::Register Parser::Register() {
    if (curtok.kind == TokenKind::ID) {
        std::string regname = lex.getId();
        auto reg = getRegister(regname);
        GetNext();
        return reg;
    } else {
        throw CreateError("Expected R");
    }
}

tiny::t86::FloatRegister Parser::FloatRegister() {
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
int64_t Parser::Imm() {
    if (IsNumber(curtok) || curtok.kind == TokenKind::MINUS) {
        int neg = 1;
        if (curtok.kind == TokenKind::MINUS) {
            neg = -1;
            GetNext();
        }
        auto val = neg * lex.getNumber();
        GetNext();
        return val;
    } else {
        throw CreateError("Expected i");
    }
}

tiny::t86::Operand Parser::FloatImm() {
    if (curtok.kind == TokenKind::FLOAT) {
        double val = lex.getFloat();
        GetNext();
        return tiny::t86::Operand(val);
    } else {
        throw CreateError("Expected f");
    }
}

tiny::t86::Operand Parser::FloatImmOrRegister() {
    if (curtok.kind == TokenKind::ID) {
        return FloatRegister();
    } else if (curtok.kind == TokenKind::FLOAT) {
        return FloatImm();
    } else {
        throw CreateError("Expected either f or F");
    }
}

/// Allows R or i
tiny::t86::Operand Parser::ImmOrRegister() {
    if (curtok.kind == TokenKind::ID) {
        return Register();
    } else if (IsNumber(curtok)) {
        return Imm();
    } else if (curtok.kind == TokenKind::MINUS) {
        if (GetNext() == TokenKind::NUM) {
            return -Imm();
        } else {
            throw CreateError("Expected either i or R");
        }
    } else {
        throw CreateError("Expected either i or R");
    }
}

/// Allows R or R [+-] i
tiny::t86::Operand Parser::ImmOrRegisterOrRegisterPlusImm() {
    if (IsNumber(curtok)) {
        return Imm();
    } else if (curtok.kind == TokenKind::ID) {
        auto reg = Register();
        if (curtok.kind == TokenKind::PLUS) {
            GetNext();
            auto imm = Imm();
            return reg + imm;
        } else if (curtok.kind == TokenKind::MINUS) {
            GetNext();
            auto imm = -Imm();
            return reg + imm;
        }
        return reg;
    } else {
        throw CreateError("Expected either i, R or R + i");
    }
}

/// Allows [i], [R], [R + i]
tiny::t86::Operand Parser::SimpleMemory() {
    if (curtok.kind == TokenKind::LBRACKET) {
        GetNext(); 
        if (curtok.kind == TokenKind::ID) {
            tiny::t86::Register inner = Register();
            if (curtok.kind == TokenKind::PLUS || curtok.kind == TokenKind::MINUS) {
                bool minus = curtok.kind == TokenKind::MINUS;
                GetNext();
                auto imm = Imm();
                if (minus) {
                    imm = -imm;
                }
                if (curtok.kind != TokenKind::RBRACKET) {
                    throw CreateError("Expected end of ']'");
                }
                GetNext();
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
tiny::t86::Operand Parser::RegisterOrSimpleMemory() {
    if (curtok.kind == TokenKind::ID) {
        return Register();
    } else if (curtok.kind == TokenKind::LBRACKET) {
        return SimpleMemory();
    } else {
        throw CreateError("Expected either R, [i], [R] or [R + i]");
    }
}

/// Allows R, i, [i], [R], [R + i]
tiny::t86::Operand Parser::ImmOrRegisterOrSimpleMemory() {
    if (curtok.kind == TokenKind::ID || IsNumber(curtok)) {
        return ImmOrRegister();
    } else if (curtok.kind == TokenKind::LBRACKET) {
        return SimpleMemory();
    } else {
        throw CreateError("Expected either i, R, [i], [R] or [R + i]");
    }
}


/// Allows R, i, R + i, [i], [R], [R + i]
tiny::t86::Operand Parser::ImmOrRegisterPlusImmOrSimpleMemory() {
    if (curtok.kind == TokenKind::ID || IsNumber(curtok)) {
        return ImmOrRegisterOrRegisterPlusImm();
    } else if (curtok.kind == TokenKind::LBRACKET) {
        return SimpleMemory();
    } else {
        throw CreateError("Expected either i, R, R + i, [i], [R] or [R + i]");
    }
}

/// Allows [i], [R], [R + i], [R1 + R2], [R1 + R2 * i],
/// [R1 + i + R2] and [R1 + i1 + R2 * i2]
tiny::t86::Operand Parser::Memory() {
    using namespace tiny::t86;
    std::optional<tiny::t86::Operand> result;
    if (curtok.kind == TokenKind::LBRACKET) {
        GetNext();
        // Must be [i]
        if (IsNumber(curtok)) {
            auto imm = lex.getNumber();
            GetNext();
            result = Mem(imm);
        } else if (curtok.kind == TokenKind::ID) {
            auto regname1 = lex.getId();
            auto reg1 = getRegister(regname1);
            GetNext();
            if (curtok.kind == TokenKind::PLUS || curtok.kind == TokenKind::MINUS) {
                bool minus = curtok.kind == TokenKind::MINUS;
                GetNext();
                // Can be [R + i], [R + i + R] or [R + i + R * i]
                if (IsNumber(curtok)) {
                    auto imm1 = Imm();
                    if (minus) {
                        imm1 = -imm1;
                    }
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
                                auto imm2 = Imm();
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
                    // Must be [R + R * i]
                    if (curtok.kind == TokenKind::TIMES) {
                        GetNext();
                        if (IsNumber(curtok)) {
                            auto imm = Imm();
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
                if (IsNumber(curtok)) {
                    auto imm = Imm();
                    result = Mem(reg1 * imm);
                } else {
                    throw CreateError("Expected 'i'");
                }
            // Must be [R]
            } else {
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

/// Parses every kind of operand excluding R + i (even floats!)
tiny::t86::Operand Parser::Operand() {
    if (curtok.kind == TokenKind::LBRACKET) {
        return Memory();
    } else if (curtok.kind == TokenKind::ID) {
        auto id = lex.getId();
        if (id.at(0) == 'F') {
            return FloatRegister();
        } else {
            return Register();
        }
    } else if (IsNumber(curtok)) {
        return Imm();
    } else if (curtok.kind == TokenKind::FLOAT) {
        return FloatImm();
    } else {
        throw CreateError("Expected operand (excluding R + i)");
    }
}

#define PARSE_BINARY(TYPE, DEST_PARSE, FROM_PARSE) \
if (ins_name == #TYPE) { \
    auto dest = DEST_PARSE(); \
    if (curtok.kind != TokenKind::COMMA) { \
        throw CreateError("Expected ','"); \
    } \
    GetNext(); \
    auto from = FROM_PARSE(); \
    return std::unique_ptr<tiny::t86::TYPE>(new tiny::t86::TYPE(std::move(dest), std::move(from))); \
}

#define PARSE_UNARY(TYPE, OPERAND_PARSE) \
if (ins_name == #TYPE) { \
    auto op = OPERAND_PARSE(); \
    return std::unique_ptr<tiny::t86::TYPE>(new tiny::t86::TYPE(std::move(op))); \
}

#define PARSE_NULLARY(TYPE) \
if (ins_name == #TYPE) { \
    return std::unique_ptr<tiny::t86::TYPE>(new tiny::t86::TYPE()); \
}

/// Parses the operands of MOV
std::unique_ptr<tiny::t86::MOV> Parser::ParseMOV() {
    // For the details of why are sometimes some
    // operands allowed see README of t86.
    auto dest = Operand();
    if (curtok.kind != TokenKind::COMMA) {
        throw CreateError("Expected ','");
    }
    GetNext();
    auto from = Operand();
    if (dest.isValue() || dest.isRegisterOffset()) {
        throw CreateError("MOV can't have i or R + i as dest");
    } else if (dest.isRegister()) {
        // Allows almost everyting
        if (from.isRegisterOffset()) {
            throw CreateError("MOV can't have R + i as from when dest is R");
        }
        if (from.isFloatValue()) {
            throw CreateError("Can't have MOV with R and f, use float register instead");
        }
    } else if (dest.isFloatRegister()) {
        if (!from.isFloatValue()
                && !from.isFloatRegister()
                && !from.isRegister()
                && !from.isMemoryImmediate()
                && !from.isMemoryRegister()) {
            throw CreateError("MOV to F can only have f, F, R, [i] or [R] as second operand, got '{}'", from.toString());
        }
    } else {
        if (!from.isRegister()
                && !from.isFloatRegister()
                && !from.isValue()) {
            throw CreateError("MOV can't have from of type '{}' when dest is '{}', allowed froms are R, F or i", dest.toString(), from.toString());
        }
    }
    return std::unique_ptr<tiny::t86::MOV>(
        new tiny::t86::MOV(std::move(dest), std::move(from)));
}

std::unique_ptr<tiny::t86::Instruction> Parser::Instruction() {
    // Address at the beginning is optional
    if (IsNumber(curtok)) {
        GetNext();
    }

    if (curtok.kind != TokenKind::ID) {
        throw CreateError("Expected register name");
    }
    std::string ins_name = lex.getId();
    GetNext();

    // MOV is a special snowflake in a sense that it allows
    // very big range of operands, but they have very restrictive
    // relationships.
    if (ins_name == "MOV") {
        return ParseMOV();
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
        return std::unique_ptr<tiny::t86::LEA>(new tiny::t86::LEA(dest, from));
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
    PARSE_UNARY(FPUSH, FloatImmOrRegister);
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
        throw CreateError("DBG instruction is not supported");
    }

    throw CreateError("Unknown instruction {}", ins_name);
}

#undef PARSE_BINARY
#undef PARSE_UNARY
#undef PARSE_NULLARY

void Parser::Text() {
    while (IsNumber(curtok) || curtok.kind == TokenKind::ID) {
        auto ins = Instruction();
        program.push_back(std::move(ins));
    }
}

void Parser::Data() {
    while (curtok.kind == TokenKind::STRING || IsNumber(curtok)) {
        if (curtok.kind == TokenKind::STRING) {
            auto string = lex.getStr();
            std::transform(string.begin(), string.end(),
                           std::back_inserter(data),
                           [](auto &&c) { return c; });
            data.push_back(0);
            GetNext();
        } else if (IsNumber(curtok)) {
            data.emplace_back(Imm());
        }
    }
}

void Parser::CheckEnd() const {
    if (curtok.kind != TokenKind::END) {
        throw CreateError("Some part of file has not been parsed (from {}:{}) due to wrong input. This can also be caused by wrong operands, ie. '.text MOV R0, R1 + 1', the 'R1 + 1' is not supported for MOV, and so it hangs in the input.", curtok.row, curtok.col);
    }
}

tiny::t86::Program Parser::Parse() {
    if (curtok.kind != TokenKind::DOT) {
        throw CreateError("File does not contain any sections");
    }
    while (curtok.kind == TokenKind::DOT) {
        GetNext();
        if (curtok.kind == TokenKind::ID
                && lex.getId() == "debug_source") {
            return {std::move(program), std::move(data)};
        }
        Section();
    }
    CheckEnd();
    return {std::move(program), std::move(data)};
}
