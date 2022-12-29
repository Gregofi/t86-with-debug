#pragma once

#include "common/colors.h"
#include "tinyC/backend/value.h"
#include "tinyC/backend/ir_visitor.h"
#include "common/colors.h"
#include "common/types.h"
#include "constant.h"
#include <algorithm>
#include <memory>
#include <set>

#define BINARY_OP_INST(className, stringName)                                                                 \
class className : public Instruction {                                                                        \
protected:                                                                                                    \
    Value* v1;                                                                                                \
    Value* v2;                                                                                                \
public:                                                                                                       \
    className(Value* v1, Value* v2) : v1(v1), v2(v2) {}                                                       \
    colors::ColorPrinter& print(colors::ColorPrinter & printer) override {                                    \
        return printer << toStringID() << " = " << (stringName) << " "                                        \
                       << v1->toStringID() << " " << v2->toStringID();                                        \
    }                                                                                                         \
    Value* getLeft() const { return v1; }                                                                     \
    Value* getRight() const { return v2; }                                                                    \
    void accept(IRVisitor& v) override {                                                                      \
        v.visit(*this);                                                                                       \
    }                                                                                                         \
    std::set<size_t> getIdsOfChildren() const override {                                                      \
        return {getID(), v1->getID(), v2->getID()};                                                           \
    }                                                                                                         \
                                                                                                              \
    void updateUsage(Value* new_val, Value* old_val) override {                                               \
        if (v1 == old_val) {                                                                                 \
            v1 = new_val;                                                                                     \
        }                                                                                                     \
        if (v2 == old_val) {                                                                                  \
            v2 = new_val;                                                                                     \
        }                                                                                                     \
    }                                                                                                         \
};

namespace tinyc {
    class Instruction: public Value {
    public:
        ~Instruction() override = default;

        /**
         * Returns ids of all instruction participating in this one, including this one (for register allocation)
         * @return
         */
        virtual std::set<size_t> getIdsOfChildren() const = 0;
    };

    BINARY_OP_INST(InsAdd, "add")

    BINARY_OP_INST(InsSub, "sub")

    BINARY_OP_INST(InsMul, "mul")

    BINARY_OP_INST(InsDiv, "div")

    BINARY_OP_INST(InsMod, "mod")

    BINARY_OP_INST(InsAnd, "and")

    BINARY_OP_INST(InsOr, "or")

    BINARY_OP_INST(InsXor, "xor")

    BINARY_OP_INST(InsLShift, "lsh")

    class InsCmp: public Instruction {
    public:
        enum class compare_op {
            LE,
            GE,
            GEQ,
            LEQ,
            EQ,
            NEQ,
        };

        static std::string opToString(compare_op op) {
            switch (op) {
                case compare_op::LE:
                    return "le";
                case compare_op::GE:
                    return "ge";
                case compare_op::GEQ:
                    return "geq";
                case compare_op::LEQ:
                    return "leq";
                case compare_op::EQ:
                    return "eq";
                case compare_op::NEQ:
                    return "neq";
                default:
                    UNREACHABLE;
            }
        }

        InsCmp(compare_op op, Value* left, Value* right) : op(op), left(left), right(right) {}

        compare_op getOp() const {
            return op;
        }

        Value* getLeft() const {
            return left;
        }

        Value* getRight() const {
            return right;
        }

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override {
            return printer << toStringID() << " = " << "cmp " << opToString(op) << " "
                           << left->toStringID() << " " << right->toStringID();
        }

        void accept(IRVisitor& v) override {
            v.visit(*this);
        }

        std::set<size_t> getIdsOfChildren() const override {
            return {getID(), left->getID(), right->getID()};
        }

        void updateUsage(Value* new_val, Value* old_val) override {
            if (right == old_val) {
                right = new_val;
            }
            if (left == old_val) {
                left = new_val;
            }
        }
    private:
        compare_op op;
        Value* left;
        Value* right;
    };

    /**
     * Allocation on the stack.
     */
    class InsAlloca: public Instruction {
        tiny::Type* type;
    public:
        explicit InsAlloca(tiny::Type* type) : type(type) {}

        tiny::Type* getType() const {
            return type;
        }

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override {
            return printer << toStringID() << " = alloca " << type->toString();
        }

        void accept(IRVisitor& v) override {
            v.visit(*this);
        }

        std::set<size_t> getIdsOfChildren() const override {
            return {getID()};
        }

        void updateUsage(Value* new_val, Value* old_val) override {
            
        }
    };

    class InsStore: public Instruction {
        Value* what;
        Value* where;
    public:
        InsStore(Value* what, Value* where) : what(what), where(where) {}

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override {
            return printer << "store - what:" << what->toStringID() << " where:" << where->toStringID();
        }

        Value* getWhat() const {
            return what;
        }

        Value* getWhere() const {
            return where;
        }

        void accept(IRVisitor& v) override {
            v.visit(*this);
        }

        std::set<size_t> getIdsOfChildren() const override {
            return {getID(), what->getID(), where->getID()};
        }

        void updateUsage(Value* new_val, Value* old_val) override {
            if (what == old_val) {
                what = new_val;
            }
            if (where == old_val) {
                where = new_val;
            }
        }
    };

    class InsLoad: public Instruction {
        Value* where;
    public:
        explicit InsLoad(Value* where) : where(where) {}

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override {
            return printer << toStringID() << " = load - where:" << where->toStringID();
        }

        Value* getWhere() const {
            return where;
        }

        void accept(IRVisitor& v) override {
            v.visit(*this);
        }

        std::set<size_t> getIdsOfChildren() const override {
            return {getID(), where->getID()};
        }

        void updateUsage(Value* new_val, Value* old_val) override {
            if (where == old_val) {
                where = new_val;
            }
        }
    };

    class InsRet: public Instruction {
        Value* val;
    public:
        explicit InsRet(Value* val) : val(val) {}

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override {
            printer << toStringID() << " = ret " << val->toStringID();
            return printer;
        }

        Value* getVal() const {
            return val;
        }

        void accept(IRVisitor& v) override {
            v.visit(*this);
        }

        std::set<size_t> getIdsOfChildren() const override {
            return {getID(), val->getID()};
        }

        void updateUsage(Value* new_val, Value* old_val) override {
            if (val == old_val) {
                val = new_val;
            }
        }
    };

    class InsCall: public Instruction {
        Value* val;
        std::vector<Value*> args;
    public:
        InsCall(Value* val, std::vector<Value*> args) : val(val), args(std::move(args)) {}

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override {
            printer << toStringID() << " = call " << val->toStringID() << " args: ";
            for (const auto& x: args) {
                printer << x->toStringID() << " ";
            }
            return printer;
        }

        Value* getVal() const {
            return val;
        }

        const std::vector<Value*>& getArgs() const {
            return args;
        }

        void accept(IRVisitor& v) override {
            v.visit(*this);
        }

        std::set<size_t> getIdsOfChildren() const override {
            std::set<size_t> res;
            for (const auto& arg: args) {
                res.emplace(arg->getID());
            }
            res.emplace(getID());
            return res;
        }

        void updateUsage(Value* new_val, Value* old_val) override {
            if (val == old_val) {
                val = new_val;
            }
            std::for_each(args.begin(), args.end(), [old_val, new_val](auto& x) {
                if (x == old_val) {
                    x = new_val;
                }
            });
        }
    };

    class InsLoadImm: public Instruction {
        Constant* c;
    public:
        explicit InsLoadImm(Constant* c) : c(c) {}

        Constant* getConstant() const {
            return c;
        }

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override {
            printer << toStringID() << " = loadimm ";
            c->print(printer);
            return printer;
        }

        void accept(IRVisitor& v) override {
            v.visit(*this);
        }

        std::set<size_t> getIdsOfChildren() const override {
            return {getID(), c->getID()};
        }

        void updateUsage(Value* new_val, Value* old_val) override {
            if (c == old_val) {
                UNREACHABLE;
                // TODO
            }
        }
    };

    class InsCondJmp: public Instruction {
        Value* cond;
        BasicBlock* true_bb;
        BasicBlock* false_bb;
    public:
        InsCondJmp(Value* cond, BasicBlock* true_bb, BasicBlock* false_bb)
                : cond(cond), true_bb(true_bb), false_bb(false_bb) {}

        Value* getCond() const {
            return cond;
        }

        BasicBlock* getTrueBb() const {
            return true_bb;
        }

        BasicBlock* getFalseBb() const {
            return false_bb;
        }

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override;

        void accept(IRVisitor& v) override {
            v.visit(*this);
        }

        std::set<size_t> getIdsOfChildren() const override;

        void updateUsage(Value* new_val, Value* old_val) override {
            if (cond == old_val) {
                cond = new_val;
            }
            // TODO: Not updating basic blocks
        }
    };

    class InsJmp: public Instruction {
        BasicBlock* dest;
    public:
        void accept(IRVisitor& v) override;

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override;

        BasicBlock* getDest() const;

        explicit InsJmp(BasicBlock* dest);

        std::set<size_t> getIdsOfChildren() const override;

        void updateUsage(Value* new_val, Value* old_val) override {
            // TODO: Not updating basic blocks
        }
    };

    class InsIndirectLoad: public Instruction {
        Value* val;
    public:
        explicit InsIndirectLoad(Value* val) : val(val) {}

        void accept(IRVisitor& v) override {
            v.visit(*this);
        }

        Value* getLoadVal() const { return val; }

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override {
            return printer << toStringID() << " = indirectLoad " << val->toStringID();
        }

        std::set<size_t> getIdsOfChildren() const override {
            return {val->getID()};
        }

        void updateUsage(Value* new_val, Value* old_val) override {
            if (val == old_val) {
                val = new_val;
            }
        }
    };

    class InsIndirectStore: public Instruction {
        Value* what;
        Value* where;
    public:
        InsIndirectStore(Value* what, Value* where) : what(what), where(where) {}

        void accept(IRVisitor& v) override {
            v.visit(*this);
        }

        Value* getWhat() const { return what; }
        Value* getWhere() const { return where; }

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override {
            return printer << "indirect store - what:" << what->toStringID() << " where: *" << where->toStringID();
        }

        std::set<std::size_t> getIdsOfChildren() const override {
            return {what->getID(), where->getID()};
        }

        void updateUsage(Value* new_val, Value* old_val) override {
            if (what == old_val) {
                what = new_val;
            }
            if (where == old_val) {
                what = new_val;
            }
        }
    };
}

