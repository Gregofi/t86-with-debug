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

struct Located {
    std::optional<expr::Location> loc;
};

struct PointerValue: Located {
    PointerValue(PointerType type, uint64_t value,
                 std::optional<expr::Location> loc = std::nullopt)
        : Located{std::move(loc)}, type(std::move(type)), value(value){}
    PointerValue() = default;
    PointerType type;
    uint64_t value;
};

struct IntegerValue: Located {
    IntegerValue(int64_t value, std::optional<expr::Location> loc = std::nullopt)
        : Located{std::move(loc)}, value(value) {}
    IntegerValue() = default;
    int64_t value;
};

struct FloatValue: Located {
    FloatValue(double value, std::optional<expr::Location> loc = std::nullopt)
        : Located{std::move(loc)}, value(value) {}
    FloatValue() = default;
    double value;
};

struct CharValue: Located {
    CharValue(char value, std::optional<expr::Location> loc = std::nullopt)
        : Located{std::move(loc)}, value(value) {}
    CharValue() = default;
    char value;
};

struct StructuredValue;
struct ArrayValue;

/// Represents evaluated typed value.
using TypedValue = std::variant<PointerValue, IntegerValue, FloatValue,
                                CharValue, StructuredValue, ArrayValue>;

struct ArrayValue: Located {
    ArrayValue(ArrayType type, uint64_t begin_address,
               std::vector<TypedValue> members,
               std::optional<expr::Location> loc = std::nullopt)
        : Located{std::move(loc)}, type(std::move(type)),
          begin_address(begin_address), members(std::move(members)) {}
    ArrayValue() = default;
    ArrayType type;
    uint64_t begin_address;
    std::vector<TypedValue> members;
};


struct StructuredValue: Located {
    StructuredValue(std::string name, uint64_t size,
                    std::map<std::string, TypedValue> members,
                    std::optional<expr::Location> loc = std::nullopt)
        : Located{std::move(loc)}, name(std::move(name)), size(size),
          members(std::move(members)) {}
    StructuredValue() = default;
    std::string name;
    uint64_t size;
    std::map<std::string, TypedValue> members;
};

class Identifier;
class EvaluatedExpr;
class ArrayAccess;
class BinaryOperator;
class MemberAccess;
class MemberDereferenceAccess;
class Float;
class Char;
class Integer;
class UnaryOperator;

/// An abstract visitor for visiting the AST of the expressions.
class ExpressionVisitor {
public:
    virtual void Visit(const Identifier&) = 0;
    virtual void Visit(const EvaluatedExpr&) = 0;
    virtual void Visit(const ArrayAccess&) = 0;
    virtual void Visit(const BinaryOperator&) = 0;
    virtual void Visit(const Float&) = 0;
    virtual void Visit(const Char&) = 0;
    virtual void Visit(const Integer&) = 0;
    virtual void Visit(const MemberAccess&) = 0;
    virtual void Visit(const MemberDereferenceAccess&) = 0;
    virtual void Visit(const UnaryOperator&) = 0;
};

/// Evaluates an expression AST.
class ExpressionEvaluator: public ExpressionVisitor {
public:
    ExpressionEvaluator(Native& native, Source& source,
                        const std::vector<TypedValue>& evaluated_expressions = {});
    void Visit(const Identifier& id) override;
    void Visit(const EvaluatedExpr& id) override;
    void Visit(const ArrayAccess& id) override;
    void Visit(const BinaryOperator& id) override;
    void Visit(const MemberAccess&) override;
    void Visit(const Integer&) override;
    void Visit(const Float&) override;
    void Visit(const Char&) override;
    void Visit(const MemberDereferenceAccess&) override;
    void Visit(const UnaryOperator&) override;
    /// Used to get result after calling Visit on the expression tree.
    TypedValue YieldResult() { return std::move(visitor_value); }
private:
    TypedValue AddValues(TypedValue&& left, TypedValue&& right);
    TypedValue SubValues(TypedValue&& left, TypedValue&& right);
    TypedValue EvaluateTypeAndLocation(const expr::Location& loc, const Type& type);
    TypedValue Assignment(TypedValue&& left, TypedValue&& right);
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

class UnaryOperator: public Expression {
public:
    enum class Op {
        Negate,
        LNot,
        Deref,
        Not,
    };
    UnaryOperator(std::unique_ptr<Expression> target, Op op): target(std::move(target)), op(op) {}
    void Accept(ExpressionVisitor& v) const override {
        v.Visit(*this);
    }
    std::unique_ptr<Expression> target;
    Op op;
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
        Assign,
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
