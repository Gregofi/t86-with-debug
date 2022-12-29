#pragma once

#include "common/colors.h"
#include "tinyC/backend/value.h"
#include "tinyC/backend/instruction.h"
#include <cstdint>
#include <vector>
#include <memory>

namespace tinyc {
    class Function;

    /**
     * @brief A container of instructions that executes sequentially.
     *
     */
    class BasicBlock: public Value, private std::vector<Instruction*> {
    protected:
        Function* parent;
    public:
        using std::vector<Instruction*>::push_back;
        using std::vector<Instruction*>::emplace_back;
        using std::vector<Instruction*>::insert;
        using std::vector<Instruction*>::erase;
        using std::vector<Instruction*>::emplace;
        using std::vector<Instruction*>::operator[];
        using std::vector<Instruction*>::begin;
        using std::vector<Instruction*>::end;
        using std::vector<Instruction*>::cbegin;
        using std::vector<Instruction*>::cend;
        using std::vector<Instruction*>::size;

        explicit BasicBlock(Function* parent = nullptr) : parent(parent) {}

        void prepend(Instruction* ins) {
            insert(begin(), ins);
        }

        void setParent(Function* parent) {
            this->parent = parent;
        }

        Function* getParent() const {
            return parent;
        }

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override {
            printer << "label " << toStringID() << ":\n";
            for (const auto& ins: *this) {
                printer << "    ";
                ins->print(printer) << "\n";
            }
            return printer;
        }

        void accept(IRVisitor& v) override {
            v.visit(*this);
        }
    };
};
