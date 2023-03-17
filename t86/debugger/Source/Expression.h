#pragma once
#include <variant>
#include <string>
#include <memory>
#include <map>
#include "Type.h"
#include "debugger/Native.h"
#include "debugger/Source/Die.h"

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

struct StructuredValue;

using TypedValue = std::variant<PointerValue, IntegerValue, FloatValue,
                                StructuredValue>;

struct StructuredValue {
    std::string name;
    uint64_t size;
    std::map<std::string, TypedValue> members;
};

inline std::string TypedValueToString(const TypedValue& v) {
    return std::visit(utils::overloaded {
        [](const PointerValue& t) {
            return fmt::format("{} = {}", TypeToString(t.type), t.value);
        },
        [](const IntegerValue& v) {
            return std::to_string(v.value);
        },
        [](const FloatValue& v) {
            return std::to_string(v.value);
        },
        [](const StructuredValue& v) {
            std::vector<std::string> members;
            for (auto&& member: v.members) {
                members.emplace_back(fmt::format(
                    "{}: {}", member.first, TypedValueToString(member.second)));
            }
            auto res = utils::join(members.begin(), members.end(), ", ");
            return fmt::format("{} = {{ {} }}", v.name, res);
        }
    }, v);
}

class Identifier;
class Dereference;
class ArrayAccess;
class Plus;
class MemberAccess;
class MemberDereferenceAccess;
class Float;
class Integer;

class ExpressionVisitor {
public:
    virtual void Visit(const Identifier&) = 0;
    virtual void Visit(const Dereference&) = 0;
    virtual void Visit(const ArrayAccess&) = 0;
    virtual void Visit(const Plus&) = 0;
    virtual void Visit(const Float&) = 0;
    virtual void Visit(const Integer&) = 0;
    virtual void Visit(const MemberAccess&) = 0;
    virtual void Visit(const MemberDereferenceAccess&) = 0;
};

class ExpressionEvaluator: public ExpressionVisitor {
public:
    ExpressionEvaluator(Native& native, Source& source);
    void Visit(const Identifier& id) override;
    void Visit(const Dereference& id) override;
    void Visit(const ArrayAccess& id) override;
    void Visit(const Plus& id) override;
    void Visit(const MemberAccess&) override;
    void Visit(const Integer&) override;
    void Visit(const Float&) override;
    void Visit(const MemberDereferenceAccess&) override;
    /// Used to get result after calling Visit on the expression tree.
    TypedValue YieldResult() { return std::move(visitor_value); }
private:
    TypedValue EvaluateTypeAndLocation(const expr::Location& loc, const Type& type);
    TypedValue Dereference(TypedValue&& val);
    std::map<std::string, const DIE*> variables;
    Native& native;
    Source& source;
    // Serves as a result since  the visit methods can't have return type.
    TypedValue visitor_value;
};

/// Represents source level expression which are used by the debugger
/// like `a + b[c]`.
class Expression {
public:
    virtual ~Expression() = default;
    virtual void Accept(ExpressionVisitor& vis) const = 0;
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

class Plus: public Expression {
public:
    Plus(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : left(std::move(left)), right(std::move(right)) { }
    void Accept(ExpressionVisitor& v) const override {
        v.Visit(*this);
    }
    std::unique_ptr<Expression> left;
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
