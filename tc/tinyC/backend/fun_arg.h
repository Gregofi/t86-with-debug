#pragma once

#include "tinyC/backend/value.h"
#include "tinyC/backend/ir_visitor.h"
#include "common/types.h"

namespace tinyc {
    class FunctionArgument: public Value {
        tiny::Type* type;
    public:
        explicit FunctionArgument(tiny::Type* type) : type(type) {}

        colors::ColorPrinter& print(colors::ColorPrinter& printer) override {
            return printer << "argument " << type->toString();
        }

        std::string toStringID() const override {
            return type->toString() + " " + Value::toStringID();
        }

        void accept(IRVisitor& v) override {
            v.visit(*this);
        }

        tiny::Type* getType() const {
            return type;
        }
    };
}
