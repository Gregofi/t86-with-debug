#pragma once

#include "common/colors.h"
#include "common/types.h"
#include "tinyC/backend/global_object.h"
#include "tinyC/backend/basic_block.h"
#include "tinyC/backend/fun_arg.h"
#include <cstdint>
#include <utility>
#include <vector>
#include <memory>

namespace tinyc {
    class Context;

    /**
     * @brief Represents single function in tinyc IR.
     * Function consist of basicblocks, symbol table and
     * list of arguments.
     */
    class Function: public GlobalObject, std::vector<BasicBlock*> {
    protected:
        tiny::Type* returns;
        std::vector<FunctionArgument*> params;

        Function(tiny::Type* ret_type, std::vector<FunctionArgument*> params, std::string id = "", bool is_main = false)
                : GlobalObject(std::move(id)), returns(ret_type), params(std::move(params)), is_main(is_main) {

        }

        bool is_main;
    public:
        using std::vector<BasicBlock*>::front;
        using std::vector<BasicBlock*>::back;
        using std::vector<BasicBlock*>::push_back;
        using std::vector<BasicBlock*>::emplace_back;
        using std::vector<BasicBlock*>::begin;
        using std::vector<BasicBlock*>::end;
        using std::vector<BasicBlock*>::size;
        using std::vector<BasicBlock*>::empty;

        static Function* CreateFunction(std::string name, tiny::Type* ret_type,
                                        std::vector<FunctionArgument*> params,
                                        Context& context, std::string id = "", bool is_main = false);

        /**
         * Adds basic block to this function and sets it's parent to this.
         * @param bb
         */
        void addBB(BasicBlock* bb) {
            bb->setParent(this);
            emplace_back(bb);
        }

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override {
            printer << "def function " << toStringID() << " (";
            for (size_t i = 0; i < params.size(); ++i) {
                printer << params[i]->toStringID();
                if (i != params.size() - 1) {
                    printer << ", ";
                }
            }
            printer << ") {\n";

            for (const auto& bb: *this) {
                bb->print(printer);
                printer << "\n";
            }
            return printer << "}\n\n";
        }

        bool hasBody() const {
            return !empty();
        }

        bool isMain() const {
            return is_main;
        }

        void accept(IRVisitor& v) override {
            v.visit(*this);
        }

        const std::vector<FunctionArgument*>& getArgs() const {
            return params;
        }
    };
}
