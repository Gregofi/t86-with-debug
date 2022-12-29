#include "tinyC/backend/value.h"

namespace tinyc {
    class IRType : public Value {
        public:

        private:
    };

    class Pointer: public Value {
        public:
            Pointer(InsAlloca* pointee) : pointee(pointee) {}
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

}
