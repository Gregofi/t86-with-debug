#include <variant>
#include <functional>
#include "Expression.h"
#include "Source.h"
#include "debugger/Source/ExpressionInterpreter.h"

/// Returns a raw value contained in given location as bytes.
/// If the location is an offset, it dereferences it.
uint64_t GetRawValue(Native& native, const expr::Location& loc) {
    return std::visit(utils::overloaded{
        [&](const expr::Register& reg) {
            return native.GetRegister(reg.name);
        },
        [&](const expr::Offset& offset) {
            return native.ReadMemory(offset.value, 1)[0];
        },
    }, loc);
}

ExpressionEvaluator::ExpressionEvaluator(Native& native, Source& source,
                        const std::vector<TypedValue>& evaluated_expressions)
    : variables(source.GetActiveVariables(native.GetIP())), 
      native(native),
      source(source),
      evaluated_expressions(evaluated_expressions) {}
 
TypedValue AddLocation(TypedValue v, expr::Location loc) {
    return std::visit(
        [&](auto&& v) -> TypedValue {
            v.loc = std::move(loc);
            return std::move(v);
        }
    , std::move(v));
}

TypedValue ExpressionEvaluator::EvaluateTypeAndLocation(const expr::Location& loc,
                                                        const Type& type) {
    TypedValue v = std::visit(utils::overloaded {
        [&](const PrimitiveType& t) -> TypedValue {
            uint64_t raw_value = GetRawValue(native, loc);
            if (t.type == PrimitiveType::Type::SIGNED) {
                return IntegerValue{utils::reinterpret_safe<int64_t>(raw_value), loc};
            } else if (t.type == PrimitiveType::Type::FLOAT) {
                return FloatValue{utils::reinterpret_safe<double>(raw_value), loc};
            } else if (t.type == PrimitiveType::Type::CHAR) {
                return CharValue{utils::reinterpret_safe<char>(raw_value), loc};
            } else {
                NOT_IMPLEMENTED;
            }
        },
        [&](const PointerType& t) -> TypedValue {
            uint64_t raw_value = GetRawValue(native, loc); 
            return PointerValue{t, raw_value, loc};
        },
        [&](const StructuredType& t) -> TypedValue {
            std::map<std::string, TypedValue> members; 
            for (auto&& member: t.members) {
                auto member_loc = ExpressionInterpreter::Interpret({
                        expr::Push{loc},
                        expr::Push{expr::Offset{member.offset}},
                        expr::Add{}}, native);
                auto member_type = source.GetType(member.type_id);
                if (member_type == nullptr) {
                    throw DebuggerError("Not enough debug information");
                }
                auto member_value = EvaluateTypeAndLocation(member_loc, *member_type);
                member_value =
                    AddLocation(std::move(member_value), std::move(member_loc));
                members.emplace(member.name, member_value);
            }
            return StructuredValue{t.name, t.size, std::move(members), loc};
        },
        [&](const ArrayType& t) -> TypedValue {
            auto* l = std::get_if<expr::Offset>(&loc);
            if (l == nullptr) {
                throw DebuggerError("Array stored in register is not supported");
            }
            auto member_type = source.GetType(t.type_id);
            if (member_type == nullptr) {
                throw DebuggerError("Unknown array subtype");
            }
            std::vector<TypedValue> members;
            for (size_t i = 0; i < t.cnt; ++i) {
                auto member_location = expr::Offset{static_cast<int64_t>(
                    l->value + i * source.GetTypeSize(t.type_id))};
                auto member = EvaluateTypeAndLocation(member_location, *member_type);
                member = AddLocation(std::move(member), std::move(member_location));
                members.emplace_back(std::move(member));
            }
            return ArrayValue{t, static_cast<uint64_t>(l->value),
                              std::move(members), loc};
        },
    }, type);
    return v;
}

void ExpressionEvaluator::Visit(const Identifier& id) {
    auto type = source.GetVariableTypeInformation(native, id.id);
    auto loc = source.GetVariableLocation(native, id.id);
    if (!loc || !type) {
        throw DebuggerError(fmt::format("Not enough type info about variable '{}'", id.id));
    }
    visitor_value = EvaluateTypeAndLocation(*loc, *type);
}

void ExpressionEvaluator::Visit(const EvaluatedExpr& e) {
    if (e.idx >= evaluated_expressions.size()) {
        throw DebuggerError(fmt::format("No expression ${}", e.idx));
    }
    visitor_value = evaluated_expressions[e.idx];
}

TypedValue ExpressionEvaluator::Dereference(TypedValue&& val) {
    if (!std::holds_alternative<PointerValue>(val)) {
        throw DebuggerError("Can only dereference pointers");
    }
    auto ptr = std::get<PointerValue>(std::move(val));
    auto* points_to_type = source.GetType(ptr.type.type_id);
    if (points_to_type == nullptr) {
        throw DebuggerError("Not enough type information");
    }
    log_info("Pointer value: {}", ptr.value);
    auto loc = expr::Offset{static_cast<int64_t>(ptr.value)};
    return EvaluateTypeAndLocation(loc, *points_to_type);
}

TypedValue ExpressionEvaluator::AddValues(TypedValue&& left, TypedValue&& right) {
    return std::visit(utils::overloaded{
        [](IntegerValue&& left, IntegerValue&& right) -> TypedValue {
            return IntegerValue{left.value + right.value}; 
        },
        [](FloatValue&& left, FloatValue&& right) -> TypedValue {
            return FloatValue{left.value + right.value};
        },
        [](CharValue&& left, CharValue&& right) -> TypedValue {
            return IntegerValue{left.value + right.value};
        },
        [this](IntegerValue&& left, PointerValue&& right) -> TypedValue {
            auto item_size = source.GetTypeSize(right.type.type_id);
            return PointerValue{right.type, left.value * item_size + right.value};
        },
        [this](PointerValue&& left, IntegerValue&& right) -> TypedValue {
            auto item_size = source.GetTypeSize(left.type.type_id);
            return PointerValue{left.type, left.value + right.value * item_size};
        },
        [](auto&&, auto&&) -> TypedValue {
            throw DebuggerError("Unsupported types for operator '+'");
        }
    }, std::move(left), std::move(right));
}

TypedValue ExpressionEvaluator::SubValues(TypedValue&& left, TypedValue&& right) {
    return std::visit(utils::overloaded{
        [](IntegerValue&& left, IntegerValue&& right) -> TypedValue {
            return IntegerValue{left.value - right.value}; 
        },
        [](FloatValue&& left, FloatValue&& right) -> TypedValue {
            return FloatValue{left.value - right.value};
        },
        [this](PointerValue&& left, IntegerValue&& right) -> TypedValue {
            auto item_size = source.GetTypeSize(left.type.type_id);
            return PointerValue{left.type, left.value - right.value * item_size};
        },
        [](PointerValue&& left, PointerValue&& right) -> TypedValue {
            if (left.type.type_id != right.type.type_id) {
                throw DebuggerError("Only pointers to the same type can be substracted");
            }
            return IntegerValue{static_cast<int64_t>(left.value - right.value)};
        },
        [](auto&&, auto&&) -> TypedValue {
            throw DebuggerError("Unsupported types for operator '-'");
        }
    }, std::move(left), std::move(right));
}

template<typename Operation>
TypedValue ArithmeticOp(TypedValue&& left, Operation&& op, TypedValue&& right) {
    return std::visit(utils::overloaded{
        [&](IntegerValue&& left, IntegerValue&& right) -> TypedValue {
            return IntegerValue{op(left.value, right.value)}; 
        },
        [&](FloatValue&& left, FloatValue&& right) -> TypedValue {
            return FloatValue{op(left.value, right.value)};
        },
        [&](CharValue&& left, CharValue&& right) -> TypedValue {
            return IntegerValue{op(left.value, right.value)}; 
        },
        [&](auto&&, auto&&) -> TypedValue {
            throw DebuggerError("Unsupported types for operator binary operator");
        }
    }, std::move(left), std::move(right));
}

template<typename Operation>
TypedValue CompareValues(TypedValue&& left, Operation&& op, TypedValue&& right) {
    return std::visit(utils::overloaded{
        [&](IntegerValue&& left, IntegerValue&& right) -> TypedValue {
            return IntegerValue{op(left.value, right.value)}; 
        },
        [&](FloatValue&& left, FloatValue&& right) -> TypedValue {
            return IntegerValue{op(left.value, right.value)};
        },
        [&](CharValue&& left, CharValue&& right) -> TypedValue {
            return IntegerValue{op(left.value, right.value)}; 
        },
        [&](PointerValue&& left, PointerValue&& right) -> TypedValue {
            return IntegerValue{op(left.value, right.value)}; 
        },
        [&](auto&&, auto&&) -> TypedValue {
            throw DebuggerError("Unsupported types for comparison operator");
        }
    }, std::move(left), std::move(right));
}

template<typename Operation>
TypedValue BitValues(TypedValue&& left, Operation&& op, TypedValue&& right) {
    return std::visit(utils::overloaded{
        [&](IntegerValue&& left, IntegerValue&& right) -> TypedValue {
            return IntegerValue{op(left.value, right.value)}; 
        },
        [&](CharValue&& left, CharValue&& right) -> TypedValue {
            return IntegerValue{op(left.value, right.value)}; 
        },
        [&](auto&&, auto&&) -> TypedValue {
            throw DebuggerError("Unsupported types for comparison operator");
        }
    }, std::move(left), std::move(right));
}

void ExpressionEvaluator::Visit(const ArrayAccess& id) {
    id.index->Accept(*this);
    auto index = std::move(visitor_value);
    id.array->Accept(*this);
    auto array = std::move(visitor_value);
    if (std::holds_alternative<PointerValue>(array)) {
        auto sum = AddValues(std::move(array), std::move(index));
        auto deref = Dereference(std::move(sum));
        visitor_value = std::move(deref);
    } else if (auto av = std::get_if<ArrayValue>(&array)) {
        auto idx = std::get_if<IntegerValue>(&index);
        if (idx == nullptr) {
            throw DebuggerError("Can only indexate with integers");
        }
        if (idx->value >= static_cast<int64_t>(av->members.size())) {
            throw DebuggerError(fmt::format("Out of bounds access: {} >= {}", idx->value, av->members.size()));
        }
        visitor_value = av->members[idx->value];
    } else {
        throw DebuggerError("Can only access arrays or pointers");
    }
}

bool CheckZero(const TypedValue& val) {
    return std::visit(utils::overloaded{
        [](const IntegerValue& v) {
            return v.value != 0;
        },
        [](const FloatValue& v) {
            return v.value != 0;
        },
        [](const CharValue& v) {
            return v.value != 0;
        },
        [](const auto&) {
            return true;
        }
    }, val);
}

template<typename UnaryOp>
TypedValue EvaluateUnary(const TypedValue& val, UnaryOp&& op) {
    return std::visit(utils::overloaded{
        [&](const IntegerValue& v) -> TypedValue {
            return IntegerValue{op(v.value)};
        },
        [&](const CharValue& v) -> TypedValue {
            return CharValue{static_cast<char>(op(v.value))};
        },
        [](const auto&) -> TypedValue {
            throw DebuggerError("Unsupported type for unary operation");
        }
    }, val);
}

void ExpressionEvaluator::Visit(const UnaryOperator& op) {
    op.target->Accept(*this);
    auto target = std::move(visitor_value);
    switch (op.op) {
    break; case UnaryOperator::Op::Deref:
        visitor_value = Dereference(std::move(target)); 
    break; case UnaryOperator::Op::Not:
        visitor_value = EvaluateUnary(target, std::bit_not{});
    break; case UnaryOperator::Op::LNot:
        visitor_value = EvaluateUnary(target, std::logical_not{});
    break; case UnaryOperator::Op::Negate:
        visitor_value = EvaluateUnary(target, std::negate{});
    }
}

void SetRaw(Native& native, const expr::Location& loc, uint64_t value) {
    std::visit(utils::overloaded {
        [&](const expr::Register& r) {
            native.SetRegister(r.name, value);
        },
        [&](const expr::Offset& r) {
            native.SetMemory(r.value, {*(int64_t*)&value});
        },
    }, loc);
}

TypedValue ExpressionEvaluator::Assignment(TypedValue&& left, TypedValue&& right) {
    std::visit([](auto&& arg) {
        if (!arg.loc) {
            throw DebuggerError("Cannot perform assignment, left expr is no lvalue");
        }
    }, left);

    return std::visit(utils::overloaded{
        [&](IntegerValue&& left, IntegerValue&& right) -> TypedValue {
            SetRaw(native, left.loc.value(), right.value);
            left.value = right.value;
            return std::move(left);
        },
        [&](FloatValue&& left, FloatValue&& right) -> TypedValue {
            SetRaw(native, left.loc.value(), right.value);
            left.value = right.value;
            return std::move(left);
        },
        [&](CharValue&& left, CharValue&& right) -> TypedValue {
            SetRaw(native, left.loc.value(), right.value);
            left.value = right.value;
            return std::move(left);
        },
        [&](PointerValue&& left, PointerValue&& right) -> TypedValue {
            if (left.type.type_id != right.type.type_id) {
                throw DebuggerError("Incopatible pointer types");
            }
            SetRaw(native, left.loc.value(), right.value);
            left.value = right.value;
            return std::move(left);
        },
        [&](ArrayValue&& left, ArrayValue&& right) -> TypedValue {
            if (left.type.type_id != right.type.type_id) {
                throw DebuggerError("Incopatible pointer types");
            }
            assert(left.members.size() == right.members.size());
            for (size_t i = 0; i < left.members.size(); ++ i) {
                left.members.at(i) = Assignment(std::move(left.members.at(i)),
                                                std::move(right.members.at(i)));
            }
            return std::move(left);
        },
        [&](StructuredValue&& left, StructuredValue&& right) -> TypedValue {
            // TODO: This is a cheap workaround, we should compare the types
            // but they are not stored in the struct.
            if (left.name != right.name) {
                throw DebuggerError("Incopatible structured types for assignment");
            }
            std::map<std::string, TypedValue> result;
            for (auto&& [name, val]: left.members) {
                if (!right.members.contains(name)) {
                    throw DebuggerError("Incompatible structured type for assignment");
                }
                result[name] = Assignment(std::move(val), std::move(right.members.at(name)));
            }
            left.members = std::move(result);
            return left;
        },
        [&](auto&&, auto&&) -> TypedValue {
            throw DebuggerError("Assignment is either not supported for given "
                    "types or they aren't of the same type.");
        }
    }, std::move(left), std::move(right));
}

void ExpressionEvaluator::Visit(const BinaryOperator& plus) {
    plus.left->Accept(*this);
    auto left = std::move(visitor_value);
    plus.right->Accept(*this);
    auto right = std::move(visitor_value);
    switch (plus.op) {
    break; case BinaryOperator::Op::Add:
        visitor_value = AddValues(std::move(left), std::move(right));
    break; case BinaryOperator::Op::Sub: 
        visitor_value = SubValues(std::move(left), std::move(right));
    break; case BinaryOperator::Op::Mul: 
        visitor_value = ArithmeticOp(std::move(left),
                                     std::multiplies{},
                                     std::move(right));
    break; case BinaryOperator::Op::Div: 
        if (!CheckZero(right)) {
            throw DebuggerError("Can't divide by zero");
        }
        visitor_value = ArithmeticOp(std::move(left),
                                     std::divides{},
                                     std::move(right));
    break; case BinaryOperator::Op::Mod: 
        if (!CheckZero(right)) {
            throw DebuggerError("Can't divide by zero");
        }
        visitor_value = BitValues(std::move(left),
                                  std::modulus{},
                                  std::move(right));
    break; case BinaryOperator::Op::Eq: 
        visitor_value = CompareValues(std::move(left),
                                      std::equal_to{},
                                      std::move(right));
    break; case BinaryOperator::Op::Neq: 
        visitor_value = CompareValues(std::move(left),
                                      std::not_equal_to{},
                                      std::move(right));
    break; case BinaryOperator::Op::Less: 
        visitor_value = CompareValues(std::move(left),
                                      std::less{},
                                      std::move(right));
    break; case BinaryOperator::Op::Greater: 
        visitor_value = CompareValues(std::move(left),
                                      std::greater{},
                                      std::move(right));
    break; case BinaryOperator::Op::Leq: 
        visitor_value = CompareValues(std::move(left),
                                      std::less_equal{},
                                      std::move(right));
    break; case BinaryOperator::Op::Geq: 
        visitor_value = CompareValues(std::move(left),
                                      std::greater_equal{},
                                      std::move(right));
    break; case BinaryOperator::Op::And: 
        visitor_value = CompareValues(std::move(left),
                                     std::logical_and{},
                                     std::move(right));
    break; case BinaryOperator::Op::Or: 
        visitor_value = CompareValues(std::move(left),
                                     std::logical_or{},
                                     std::move(right));
    break; case BinaryOperator::Op::IAnd: 
        visitor_value = BitValues(std::move(left),
                                  std::bit_and{},
                                  std::move(right));
    break; case BinaryOperator::Op::IOr: 
        visitor_value = BitValues(std::move(left),
                                  std::bit_or{},
                                  std::move(right));
    break; case BinaryOperator::Op::IXor: 
        visitor_value = BitValues(std::move(left),
                                  std::bit_xor{},
                                  std::move(right));
    break; case BinaryOperator::Op::LShift: 
        visitor_value = BitValues(std::move(left),
                                  [](auto&& l, auto&& r) { return l << r; },
                                  std::move(right));
    break; case BinaryOperator::Op::RShift: 
        visitor_value = BitValues(std::move(left),
                                  [](auto&& l, auto&& r) { return l >> r; },
                                  std::move(right));
    break; case BinaryOperator::Op::Assign: 
        visitor_value = Assignment(std::move(left), std::move(right));
    break; default: UNREACHABLE;
    }
}

TypedValue AccessMember(const TypedValue& base, const std::string& member) {
    if (!std::holds_alternative<StructuredValue>(base)) {
        throw DebuggerError("Member access can only be used on structured values");
    }
    auto structured = std::get<StructuredValue>(base);
    if (!structured.members.contains(member)) {
        throw DebuggerError(fmt::format("The '{}' doesn't have '{}' member",
                                        structured.name, member));
    }
    return structured.members.at(member);
}

void ExpressionEvaluator::Visit(const MemberAccess& access) {
    access.base->Accept(*this);
    auto res = AccessMember(visitor_value, access.member);
    visitor_value = std::move(res);
}

void ExpressionEvaluator::Visit(const Integer& integer) {
    visitor_value = IntegerValue{integer.value};
}

void ExpressionEvaluator::Visit(const Float& fl) {
    visitor_value = FloatValue{fl.value};
}

void ExpressionEvaluator::Visit(const Char& c) {
    visitor_value = CharValue{c.value};
}

void ExpressionEvaluator::Visit(const MemberDereferenceAccess& m) {
    m.base->Accept(*this);
    auto base = std::move(visitor_value);
    auto dereferenced = Dereference(std::move(base));
    auto member = AccessMember(dereferenced, m.member); 
    visitor_value = std::move(member);
}
