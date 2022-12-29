#pragma once

#include "tinyC/backend/basic_block.h"
#include "tinyC/backend/function.h"
#include "tinyC/backend/value.h"
#include "tinyC/backend/instruction.h"
#include "tinyC/backend/basic_block.h"
#include "tinyC/backend/pointers.h"
#include <vector>
#include <cassert>

/**
 * Creates function that generates instruction 'insName', name of the function is 'funName'.
 * Is intended to generate binary operations, like addition, multiplications and so on...
 */
#define CREATE_INS_BINOP(funName, insName)                                                                 \
insName* funName(Value* v1, Value *v2) {                                                                   \
    assert(insertionPoint != nullptr);                                                                     \
    auto ins = new insName{v1, v2};                                                                        \
    insertionPoint->push_back(ins);                                                                        \
    return ins;                                                                                            \
}


namespace tinyc {
    static const unsigned INT_SIZE = 4;
    static const unsigned DOUBLE_SIZE = 8;

    class IRBuilder {
        BasicBlock* insertionPoint;

    public:
        void SetInsertionPoint(BasicBlock* bb) {
            insertionPoint = bb;
        }

        BasicBlock* getInsertionPoint() const {
            return insertionPoint;
        }

        /* ---------------------------
         *     Binary operations.
         * ---------------------------
         */
        CREATE_INS_BINOP(createIAdd, InsAdd)

        CREATE_INS_BINOP(createISub, InsSub)

        CREATE_INS_BINOP(createIMul, InsMul)

        CREATE_INS_BINOP(createIDiv, InsDiv)

        CREATE_INS_BINOP(createIMod, InsMod)

        /**
         * Creates alloca instruction AT THE BEGINNING of current insert point.
         * @param type - type to allocate memory to
         * @return - Alloca instruction
         */
        InsAlloca* allocateType(tiny::Type* type) {
            auto alloca = new InsAlloca(type);
            insertionPoint->prepend(alloca);
            return alloca;
        }

        InsRet* createIRet(Value* v1) {
            auto ret = new InsRet(v1);
            insertionPoint->push_back(ret);
            return ret;
        }

        InsCall* createICall(Function* fun, std::vector<Value*> args) {
            auto call = new InsCall(std::move(fun), std::move(args));
            insertionPoint->push_back(call);
            return call;
        }

        InsStore* createIStore(Value* what, Value* where) {
            if (!dynamic_cast<InsAlloca*>(where)) {
                std::cerr << "Destination of store instruction must be alloca" << std::endl;
                exit(1);
            }
            auto store = new InsStore(std::move(what), std::move(where));
            insertionPoint->push_back(store);
            return store;
        }

        InsIndirectStore* createIndirectStore(Value* what, Value* where) {
            auto store = new InsIndirectStore(what, where);
            insertionPoint->push_back(store);
            return store;
        }

        InsLoad* createILoad(Value* from_where) {
            if (!dynamic_cast<InsAlloca*>(from_where) && !dynamic_cast<IRPointer*>(from_where)) {
                std::cerr << "Load instruction can only receive alloca as it's argument" << std::endl;
                exit(1);
            }
            auto* store = new InsLoad(from_where);
            insertionPoint->push_back(store);
            return store;
        }

        InsIndirectLoad* createIndirectLoad(Value* from_where) {
            auto* load = new InsIndirectLoad(from_where);
            insertionPoint->push_back(load);
            return load;
        }

        InsLoadImm* createILoadImm(Constant* c) {
            auto* loadimm = new InsLoadImm(c);
            insertionPoint->push_back(loadimm);
            return loadimm;
        }

        InsCondJmp* createICondJmp(Value* cond, BasicBlock* if_bb, BasicBlock* else_bb) {
            auto* condjmp = new InsCondJmp(cond, if_bb, else_bb);
            insertionPoint->push_back(condjmp);
            return condjmp;
        }

        InsJmp* createIJmp(BasicBlock* dest) {
            auto* jmp = new InsJmp(dest);
            insertionPoint->push_back(jmp);
            return jmp;
        }

        InsCmp* createICmp(InsCmp::compare_op op, Value* left, Value* right) {
            auto* cmp = new InsCmp(op, left, right);
            insertionPoint->push_back(cmp);
            return cmp;
        }
    };
}
