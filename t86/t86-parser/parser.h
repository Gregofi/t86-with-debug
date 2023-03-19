#pragma once
#include <optional>
#include <variant>
#include <iostream>
#include <cstring>
#include <string>
#include <string_view>
#include "fmt/core.h"
#include "common/parsing.h"
#include "t86/program/programbuilder.h"
#include "t86/cpu.h"
#include "t86/program/helpers.h"
#include "common/helpers.h"
#include "common/logger.h"

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

    /// Returns true if the current token represents a number.
    /// In a normal world where expression are arbitrary, the 
    /// parse would be covered by an unary operator expression.
    /// The T86 however places strict conditions on the expressions,
    /// and so we have to check the lookahead like this.
    bool IsNumber(Token tk) {
        return tk.kind == TokenKind::NUM || tk.kind == TokenKind::MINUS;
    }

    void Text();

    void Data();

    tiny::t86::Program Parse();

    template<typename ...Args>
    ParserError CreateError(fmt::format_string<Args...> format, Args&& ...args) const {
        return ParserError(fmt::format("Error:{}:{}:{}", curtok.row,
                    curtok.col, fmt::format(format,
                        std::forward<Args>(args)...)));
    }

    void CheckEnd() const;
private:
    TokenKind GetNext();

    Lexer lex;
    Token curtok;
    std::vector<std::unique_ptr<tiny::t86::Instruction>> program;
    std::vector<int64_t> data;
};

