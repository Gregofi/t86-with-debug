#pragma once

namespace tinyc {
    class Instruction;

    class InsAdd;

    class InsSub;

    class InsMul;

    class InsMod;

    class InsAnd;

    class InsOr;

    class InsXor;

    class InsLShift;

    class InsAlloca;

    class InsStore;

    class InsLoad;

    class InsRet;

    class InsCall;

    class InsLoadImm;

    class Function;

    class BasicBlock;

    class Value;

    class FunctionArgument;

    class InsCmp;

    class InsJmp;

    class InsCondJmp;

    class InsIndirectStore;

    class InsIndirectLoad;

    class IRVisitor {
    public:
        virtual ~IRVisitor() = default;

        virtual void visit(const InsAdd& ins) = 0;

        virtual void visit(const InsSub& ins) = 0;

        virtual void visit(const InsMul& ins) = 0;

        virtual void visit(const InsMod& ins) = 0;

        virtual void visit(const InsAnd& ins) = 0;

        virtual void visit(const InsOr& ins) = 0;

        virtual void visit(const InsXor& ins) = 0;

        virtual void visit(const InsAlloca& ins) = 0;

        virtual void visit(const InsStore& ins) = 0;

        virtual void visit(const InsLoad& ins) = 0;

        virtual void visit(const InsRet& ins) = 0;

        virtual void visit(const InsCall& ins) = 0;

        virtual void visit(const InsLoadImm& ins) = 0;

        virtual void visit(const Function& fun) = 0;

        virtual void visit(const BasicBlock& bb) = 0;

        virtual void visit(const Instruction& ins) = 0;

        virtual void visit(const FunctionArgument& ins) = 0;

        virtual void visit(const InsCmp& ins) = 0;

        virtual void visit(const InsJmp& ins) = 0;

        virtual void visit(const InsCondJmp& ins) = 0;

        virtual void visit(const InsIndirectStore& ins) = 0;

        virtual void visit(const InsIndirectLoad& ins) = 0;

        virtual void visit(const InsLShift& ins) = 0;
    };
}
