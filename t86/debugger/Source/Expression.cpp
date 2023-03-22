#include <variant>
#include "Expression.h"
#include "Source.h"
#include "debugger/Source/ExpressionInterpreter.h"

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
 
TypedValue ExpressionEvaluator::EvaluateTypeAndLocation(const expr::Location& loc,
                                                        const Type& type) {
    TypedValue v = std::visit(utils::overloaded {
        [&](const PrimitiveType& t) -> TypedValue {
            uint64_t raw_value = GetRawValue(native, loc);
            if (t.type == PrimitiveType::Type::SIGNED) {
                return IntegerValue{*reinterpret_cast<int64_t*>(&raw_value)};
            } else if (t.type == PrimitiveType::Type::FLOAT) {
                return FloatValue{*reinterpret_cast<double*>(&raw_value)};
            } else if (t.type == PrimitiveType::Type::CHAR) {
                return CharValue{*reinterpret_cast<char*>(&raw_value)};
            } else {
                NOT_IMPLEMENTED;
            }
        },
        [&](const PointerType& t) -> TypedValue {
            uint64_t raw_value = GetRawValue(native, loc); 
            return PointerValue{t, raw_value};
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
                members.emplace(member.name, member_value);
            }
            return StructuredValue{t.name, t.size, std::move(members)};
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

void ExpressionEvaluator::Visit(const class Dereference& deref) {
    deref.target->Accept(*this);
    auto value = std::move(visitor_value);
    visitor_value = Dereference(std::move(value));
}

TypedValue AddValues(TypedValue&& left, TypedValue&& right) {
    return std::visit(utils::overloaded{
        [&](IntegerValue&& left, IntegerValue&& right) -> TypedValue {
            return IntegerValue{left.value + right.value}; 
        },
        [&](FloatValue&& left, FloatValue&& right) -> TypedValue {
            return FloatValue{left.value + right.value};
        },
        [&](IntegerValue&& left, PointerValue&& right) -> TypedValue {
            return PointerValue{right.type, left.value + right.value};
        },
        [&](PointerValue&& left, IntegerValue&& right) -> TypedValue {
            return PointerValue{left.type, left.value + right.value};
        },
        [&](auto&&, auto&&) -> TypedValue {
            throw DebuggerError("Unsupported types for operator '+'");
        }
    }, std::move(left), std::move(right));
}

void ExpressionEvaluator::Visit(const ArrayAccess& id) {
    id.index->Accept(*this);
    auto index = std::move(visitor_value);
    id.array->Accept(*this);
    auto array = std::move(visitor_value);
    auto sum = AddValues(std::move(array), std::move(index));
    auto deref = Dereference(std::move(sum));
    visitor_value = std::move(deref);
}

void ExpressionEvaluator::Visit(const Plus& plus) {
    plus.left->Accept(*this);
    auto left = std::move(visitor_value);
    plus.right->Accept(*this);
    auto right = std::move(visitor_value);
    visitor_value = AddValues(std::move(left), std::move(right));
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
