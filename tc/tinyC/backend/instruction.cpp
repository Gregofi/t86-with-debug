#include "tinyC/backend/instruction.h"
#include "tinyC/backend/basic_block.h"
#include "tinyC/backend/value.h"

namespace tinyc {
    colors::ColorPrinter& tinyc::InsCondJmp::print(colors::ColorPrinter& printer) {
        return printer << toStringID() << " = condjmp, cond: " << cond->toStringID() << ", true: "
                       << true_bb->toStringID() << ", false: " << false_bb->toStringID();
    }

    std::set<size_t> InsCondJmp::getIdsOfChildren() const {
        return {getID(), cond->getID(), true_bb->getID(), false_bb->getID()};
    }

    tinyc::InsJmp::InsJmp(tinyc::BasicBlock* dest) : dest(dest) {}

    BasicBlock* InsJmp::getDest() const {
        return dest;
    }

    void InsJmp::accept(IRVisitor& v) {
        v.visit(*this);
    }

    colors::ColorPrinter& InsJmp::print(colors::ColorPrinter& printer) {
        return printer << toStringID() << " = jmp " << dest->toStringID();
    }

    std::set<size_t> InsJmp::getIdsOfChildren() const {
        return std::set<size_t>();
    }
}
