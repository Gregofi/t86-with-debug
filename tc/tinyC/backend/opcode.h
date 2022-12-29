#pragma once
#include <memory>
#include <string>
#include <vector>
#include "operand.h"
#include "../../common/libs/magic_enum.h"

class T86Ins {
public:
    enum class Opcode {
        ADD,
        MOV,
        MUL,
        HALT,
        CALL,
        RET,
        NOP,
        SUB,
        XOR,
        AND,
        OR,
        MOD,
        PUSH,
        POP,
        JMP,
        JE,
        JNE,
        JL,
        JZ,
        JNZ,
        JG,
        JGE,
        JLE,
        CMP,
        LEA,
        PUTNUM,
        LSH,
    };
    T86Ins(Opcode opcode, std::vector<Operand> operands): opcode(opcode), operands(std::move(operands)) { }
    Opcode opcode;
    std::vector<Operand> operands;
    std::string toString() const {
        std::string res = std::string{magic_enum::enum_name(opcode)};
        bool first = true;
        for (const auto operand: operands) {
            res += (first ? " " : ",") + std::visit([](auto&& arg){return arg.toString();}, operand);
            first = false;
        }
        return res;
    }
};

class Program : public std::vector<T86Ins> {
public:
    template<typename ...Args>
    size_t add_ins(T86Ins::Opcode opcode, Args ...args) {
        auto idx = size();
        std::vector<Operand> operands;
        operands.reserve(sizeof...(args));
        (operands.emplace_back(args), ...);
        emplace_back(opcode, std::move(operands));
        return idx;
    }

    void patch(size_t idx, size_t dest) {
        at(idx).operands.at(0) = dest;
    }
};
