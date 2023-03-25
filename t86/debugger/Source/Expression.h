#pragma once
#include <variant>
#include <string>
#include <memory>
#include <map>
#include "Type.h"
#include "debugger/Native.h"
#include "debugger/Source/Die.h"

// Contains classes and functions used to evaluate
// debugger expressions (like a + b->c).
// The expression is converted into an AST and evaluated
// by visitor class.

class Source;

struct PointerValue {
    PointerType type;
    uint64_t value;
};

struct IntegerValue {
    int64_t value;
};

struct FloatValue {
    double value;
};

struct CharValue {
    char value;
};

struct StructuredValue;

/// Represents evaluated typed value.
using TypedValue = std::variant<PointerValue, IntegerValue, FloatValue,
                                CharValue, StructuredValue>;

struct StructuredValue {
    std::string name;
    uint64_t size;
    std::map<std::string, TypedValue> members;
};

class Identifier;
class EvaluatedExpr;
class Dereference;
class ArrayAccess;
class BinaryOperator;
class MemberAccess;
class MemberDereferenceAccess;
class Float;
class Char;
class Integer;

/// An abstract visitor for visiting the AST of the expressions.
class ExpressionVisitor {
public:
    virtual void Visit(const Identifier&) = 0;
    virtual void Visit(const EvaluatedExpr&) = 0;
    virtual void Visit(const Dereference&) = 0;
    virtual void Visit(const ArrayAccess&) = 0;
    virtual void Visit(const BinaryOperator&) = 0;
    virtual void Visit(const Float&) = 0;
    virtual void Visit(const Char&) = 0;
    virtual void Visit(const Integer&) = 0;
    virtual void Visit(const MemberAccess&) = 0;
    virtual void Visit(const MemberDereferenceAccess&) = 0;
};

/// Evaluates an expression AST.
class ExpressionEvaluator: public ExpressionVisitor {
public:
    ExpressionEvaluator(Native& native, Source& source,
                        const std::vector<TypedValue>& evaluated_expressions = {});
    void Visit(const Identifier& id) override;
    void Visit(const EvaluatedExpr& id) override;
    void Visit(const Dereference& id) override;
    void Visit(const ArrayAccess& id) override;
    void Visit(const BinaryOperator& id) override;
    void Visit(const MemberAccess&) override;
    void Visit(const Integer&) override;
    void Visit(const Float&) override;
    void Visit(const Char&) override;
    void Visit(const MemberDereferenceAccess&) override;
    /// Used to get result after calling Visit on the expression tree.
    TypedValue YieldResult() { return std::move(visitor_value); }
private:
    TypedValue EvaluateTypeAndLocation(const expr::Location& loc, const Type& type);
    TypedValue Dereference(TypedValue&& val);
    std::map<std::string, const DIE*> variables;
    Native& native;
    Source& source;
    const std::vector<TypedValue>& evaluated_expressions;
    // Serves as a result since the visit methods can't have return type.
    TypedValue visitor_value;
};

/// Represents source level expression which are used by the debugger
/// like `a + b[c]`.
class Expression {
public:
    virtual ~Expression() = default;
    virtual void Accept(ExpressionVisitor& vis) const = 0;
};

class EvaluatedExpr: public Expression {
public:
    EvaluatedExpr(size_t idx): idx(idx) { }
    virtual void Accept(ExpressionVisitor& v) const override {
        v.Visit(*this);
    }
    size_t idx;
};

class Identifier: public Expression {
public:
    Identifier(std::string id): id(std::move(id)) {}
    void Accept(ExpressionVisitor& v) const override {
        v.Visit(*this);
    }
    std::string id;
};

class Dereference: public Expression {
public:
    Dereference(std::unique_ptr<Expression> target): target(std::move(target)) {}
    void Accept(ExpressionVisitor& v) const override {
        v.Visit(*this);
    }
    std::unique_ptr<Expression> target;
};

class ArrayAccess: public Expression {
public:
    ArrayAccess(std::unique_ptr<Expression> array, std::unique_ptr<Expression> index)
        : array(std::move(array)), index(std::move(index)) { }
    void Accept(ExpressionVisitor& v) const override {
        v.Visit(*this);
    }
    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;
};

class BinaryOperator: public Expression {
public:
    enum class Op {
        Add,
        Sub,
        Mul,
        Div,
        Mod,
        Eq,
        Neq,
        Leq,
        Geq,
        Greater,
        Less,
        And,
        Or,
        IAnd,
        IOr,
        IXor,
        LShift,
        RShift,
    };
    BinaryOperator(std::unique_ptr<Expression> left, Op op,
            std::unique_ptr<Expression> right)
        : left(std::move(left)), op(op), right(std::move(right)) { }
    void Accept(ExpressionVisitor& v) const override {
        v.Visit(*this);
    }
    std::unique_ptr<Expression> left;
    Op op;
    std::unique_ptr<Expression> right;
};

/// The '.' operator
class MemberAccess: public Expression {
public:
    MemberAccess(std::unique_ptr<Expression> base,
                 std::string member)
        : base(std::move(base)), member(std::move(member)) { }
    void Accept(ExpressionVisitor& v) const override {
        v.Visit(*this);
    }
    std::unique_ptr<Expression> base;
    std::string member;
};

/// The '->' operator
class MemberDereferenceAccess: public Expression {
public:
    MemberDereferenceAccess(std::unique_ptr<Expression> base,
                            std::string member)
        : base(std::move(base)), member(std::move(member)) { }
    void Accept(ExpressionVisitor& v) const override {
        v.Visit(*this);
    }
    std::unique_ptr<Expression> base;
    std::string member;
};

class Integer: public Expression {
public:
    Integer(int64_t value): value(value) {}
    void Accept(ExpressionVisitor& v) const override {
        v.Visit(*this);
    }
    int64_t value;
};

class Float: public Expression {
public:
    Float(double value): value(value) {}
    void Accept(ExpressionVisitor& v) const override {
        v.Visit(*this);
    }
    double value;
};

class Char: public Expression {
public:
    Char(char value): value(value) {}
    void Accept(ExpressionVisitor& v) const override {
        v.Visit(*this);
    }
    char value;
};
