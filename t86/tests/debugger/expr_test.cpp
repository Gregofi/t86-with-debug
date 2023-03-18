#include <gtest/gtest.h>
#include <memory>
#include "debugger/Source/ExpressionInterpreter.h"
#include "debugger/Source/LocExpr.h"
#include "utils.h"

using namespace expr;
TEST(LocationExpr, SimpleExpression) {
    std::vector<expr::LocExpr> exprs = {
        Push{Offset{1}},
        Push{Offset{3}},
        Add{},
    };
    // Can't use make_unique with init lists :(
    std::unique_ptr<MockedProcess> p{new MockedProcess({}, {}, {})};
    Native n(std::move(p));
    auto res = ExpressionInterpreter::Interpret(exprs, n);
    ASSERT_EQ(std::get<Offset>(res).value, 4);
}

TEST(LocationExpr, FrameBaseOffset) {
    std::vector<expr::LocExpr> exprs = {
        FrameBaseRegisterOffset{-4}, 
    };
    // Can't use make_unique with init lists :(
    std::unique_ptr<MockedProcess> p{new MockedProcess({}, {}, {{"BP", 20}})};
    Native n(std::move(p));
    auto res = ExpressionInterpreter::Interpret(exprs, n);
    ASSERT_EQ(std::get<Offset>(res).value, 16);
}

TEST(LocationExpr, RegResult) {
    std::vector<expr::LocExpr> exprs = {
        Push{Register{"R0"}}, 
    };
    // Can't use make_unique with init lists :(
    std::unique_ptr<MockedProcess> p{new MockedProcess({}, {}, {{"BP", 20}})};
    Native n(std::move(p));
    auto res = ExpressionInterpreter::Interpret(exprs, n);
    ASSERT_EQ(std::get<Register>(res).name, "R0");
}

TEST(LocationExpr, Dereference) {
    std::vector<expr::LocExpr> exprs = {
        Push{Register{"R0"}}, 
        expr::Dereference{},
    };
    std::map<std::string, int64_t> regs = {
        {"R0", 2},
        {"BP", 20}
    };
    // Can't use make_unique with init lists :(
    std::unique_ptr<MockedProcess> p{new MockedProcess({}, {1,2,3,4,5}, regs)};
    Native n(std::move(p));
    auto res = ExpressionInterpreter::Interpret(exprs, n);
    ASSERT_EQ(std::get<Offset>(res).value, 3);
}
