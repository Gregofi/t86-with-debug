#include <variant>
#include <map>
#include <vector>
#include <cstdint>
#include "debugger/Native.h"
#include "debugger/Source/ExpressionInterpreter.h"
#include "debugger/Source/LocExpr.h"
#include "common/helpers.h"

expr::Location ExpressionInterpreter::Interpret(const std::vector<expr::LocExpr>& expr,
                                               Native& native,
                                               std::string frame_base_reg_name) {
    ExpressionInterpreter vm(expr, native, std::move(frame_base_reg_name));
    vm.Interpret();
    if (vm.s.empty()) {
        throw InterpretError("Empty stack at the end of calculation");
    }
    return vm.s.top();
}

expr::Location ExpressionInterpreter::AddOperands(const expr::Location& o1,
                                                 const expr::Location& o2) const {
    return std::visit(utils::overloaded {
        [&](const expr::Offset& i1) {
            return std::visit(utils::overloaded {
                [&](const expr::Offset& i2) {
                    return expr::Offset{i1.value + i2.value};
                },
                [&](const expr::Register& r2) {
                    auto rval2 = native.GetRegister(r2.name);
                    return expr::Offset{i1.value + rval2};
                },
            }, o2);
        },
        [&](const expr::Register& r1) {
            auto rval1 = native.GetRegister(r1.name);
            return std::visit(utils::overloaded {
                [&](const expr::Offset& i2) {
                    return expr::Offset{rval1 + i2.value};
                },
                [&](const expr::Register& r2) {
                    auto rval2 = native.GetRegister(r2.name);
                    return expr::Offset{rval1 + rval2};
                },
            }, o2);
        },
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
                s.push(expr::Offset{loc});
            },
            [&](const expr::Dereference& ins) {
                auto v1 = s.top();
                auto v = std::visit(utils::overloaded {
                        [&](const expr::Register& reg) {
                            auto idx = native.GetRegister(reg.name);
                            return native.ReadMemory(idx, 1)[0];
                        },
                        [&](const expr::Offset& offset) {
                            auto idx = native.ReadMemory(offset.value, 1)[0];
                            return native.ReadMemory(idx, 1)[0];
                        }
                }, v1);
                s.push(expr::Offset{v});
            }
        }, ins);
    }
}
