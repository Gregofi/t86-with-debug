#include <variant>
#include <map>
#include <vector>
#include <cstdint>
#include "debugger/Native.h"
#include "debugger/Source/ExpressionInterpreter.h"
#include "debugger/Source/LocExpr.h"
#include "common/helpers.h"

expr::Operand ExpressionInterpreter::Interpret(const std::vector<expr::LocExpr>& expr,
                                               Native& native,
                                               std::string frame_base_reg_name) {
    ExpressionInterpreter vm(expr, native, std::move(frame_base_reg_name));
    vm.Interpret();
    if (vm.s.empty()) {
        throw InterpretError("Empty stack at the end of calculation");
    }
    return vm.s.top();
}

expr::Operand ExpressionInterpreter::AddOperands(const expr::Operand& o1,
                                                 const expr::Operand& o2) const {
    return std::visit(utils::overloaded {
        [&](const expr::Integer& i1) {
            return std::visit(utils::overloaded {
                [&](const expr::Integer& i2) {
                    return expr::Integer{i1.value + i2.value};
                },
                [&](const expr::Register& r2) {
                    auto rval2 = native.GetRegister(r2.name);
                    return expr::Integer{i1.value + rval2};
                },
            }, o2);
        },
        [&](const expr::Register& r1) {
            auto rval1 = native.GetRegister(r1.name);
            return std::visit(utils::overloaded {
                [&](const expr::Integer& i2) {
                    return expr::Integer{rval1 + i2.value};
                },
                [&](const expr::Register& r2) {
                    auto rval2 = native.GetRegister(r2.name);
                    return expr::Integer{rval1 + rval2};
                },
            }, o2);
        }
    }, o1);
}

void ExpressionInterpreter::Interpret() {
    for (const auto& ins: exprs) {
        std::visit(utils::overloaded {
            [&](const expr::Push& ins) {
                s.push(ins.value);
            },
            [&](const expr::Add& ins) {
                auto v1 = s.top();
                s.pop();
                auto v2 = s.top();
                s.pop();
                auto res = AddOperands(v1, v2);
                s.push(std::move(res));
            },
            [&](const expr::FrameBaseRegisterOffset& ins) {
                auto bp = native.GetRegister(frame_base_reg_name);
                auto loc = bp + ins.offset;
                s.push(expr::Integer{loc});
            },
        }, ins);
    }
}
