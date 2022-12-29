#pragma once

#include "tinyC/backend/value.h"
#include "tinyC/backend/instruction.h"

namespace tinyc {
    class IRPointer: public Value {
        public:
            IRPointer(InsAlloca* pointee) : pointee(pointee) {}
            InsAlloca* getPointee() { return pointee; }
            void accept(IRVisitor& v) override {
                UNREACHABLE;
            }
            colors::ColorPrinter& print(colors::ColorPrinter& printer) override {
                printer << "*" << pointee->toStringID();
                return printer;
            }
        private:
            InsAlloca* pointee;

    };
} // namespace
