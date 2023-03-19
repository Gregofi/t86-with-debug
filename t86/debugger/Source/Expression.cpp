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

std::string TypedValueTypeToString(const TypedValue& v) {
    using std::string_literals::operator""s;
    return std::visit(utils::overloaded {
        [](const PointerValue& t) {
            return fmt::format("{}", TypeToString(t.type));
        },
        [](const IntegerValue& v) {
            return "int"s;
        },
        [](const FloatValue& v) {
            return "float"s;
        },
        [](const StructuredValue& v) {
            return v.name;
        }
    }, v);
}

std::string TypedValueToString(const TypedValue& v) {
    return std::visit(utils::overloaded {
        [](const PointerValue& t) {
            return fmt::format("{}", t.value);
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
                    "{}: {} = {}", member.first,
                    TypedValueTypeToString(member.second),
                    TypedValueToString(member.second)));
            }
            auto res = utils::join(members.begin(), members.end(), ", ");
            return fmt::format("{{ {} }}", res);
        }
    }, v);
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
                if (!member.type) {
                    throw DebuggerError("Not enough debug information");
                }
                auto member_loc = ExpressionInterpreter::Interpret({
                        expr::Push{loc},
                        expr::Push{expr::Offset{member.offset}},
                        expr::Add{}}, native);
                auto member_value = EvaluateTypeAndLocation(member_loc, *member.type);
                members.emplace(member.name, member_value);
            }
            return StructuredValue{t.name, t.size, std::move(members)};
        },
    }, type);
    return v;
}

void ExpressionEvaluator::Visit(const Identifier& id) {
    if (!variables.contains(id.id)) {
        throw DebuggerError("Not enough debug information");
    }
    auto die = variables.at(id.id);
    auto loc = FindDieAttribute<ATTR_location_expr>(*die);
    auto type_die = FindDieAttribute<ATTR_type>(*die);
    if (!loc || !type_die) {
        throw DebuggerError("Not enough debug information");
    }
    
    auto interpreted_loc = ExpressionInterpreter::Interpret(loc->locs, native);
    auto type = source.ReconstructTypeInformation(type_die->type_id);
    if (!type) {
        throw DebuggerError("Not enough debug information");
    }
    visitor_value = EvaluateTypeAndLocation(interpreted_loc, *type);
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
    auto points_to_type = source.ReconstructTypeInformation(ptr.type.type_idx);
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

void ExpressionEvaluator::Visit(const MemberDereferenceAccess& m) {
    m.base->Accept(*this);
    auto base = std::move(visitor_value);
    auto dereferenced = Dereference(std::move(base));
    auto member = AccessMember(dereferenced, m.member); 
    visitor_value = std::move(member);
}
