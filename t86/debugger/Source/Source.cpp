#include "debugger/Source/Source.h"
#include "debugger/Source/ExpressionInterpreter.h"
#include "debugger/Source/LineMapping.h"

uint64_t Source::SetSourceSoftwareBreakpoint(Native& native, size_t line) const {
    auto mapping = UnwrapOptional(line_mapping, "No debug info for line mapping");
    auto addr =
        UnwrapOptional(mapping.GetAddress(line),
                       fmt::format("No debug info for line '{}'", line));
    native.SetBreakpoint(addr);
    return addr;
}

uint64_t Source::UnsetSourceSoftwareBreakpoint(Native& native, size_t line) const {
    auto mapping = UnwrapOptional(line_mapping, "No debug info for line mapping");
    auto addr =
        UnwrapOptional(mapping.GetAddress(line),
                       fmt::format("No debug info for line '{}'", line));
    native.UnsetBreakpoint(addr);
    return addr;
}

uint64_t Source::EnableSourceSoftwareBreakpoint(Native& native, size_t line) const {
    auto mapping = UnwrapOptional(line_mapping, "No debug info for line mapping");
    auto addr =
        UnwrapOptional(mapping.GetAddress(line),
                       fmt::format("No debug info for line '{}'", line));
    native.EnableSoftwareBreakpoint(addr);
    return addr;
}

uint64_t Source::DisableSourceSoftwareBreakpoint(Native& native, size_t line) const {
    auto mapping = UnwrapOptional(line_mapping, "No debug info for line mapping");
    auto addr =
        UnwrapOptional(mapping.GetAddress(line),
                       fmt::format("No debug info for line '{}'", line));
    native.DisableSoftwareBreakpoint(addr);
    return addr;
}

const std::vector<std::string>& Source::GetLines() const {
    if (!source_file) {
        throw DebuggerError("No debug information about source code");
    }
    return source_file->GetLines();
}

std::optional<size_t> Source::AddrToLine(size_t addr) const {
    if (!line_mapping) {
        return {};
    }
    auto lines = line_mapping->GetLines(addr);
    auto max = std::max_element(lines.begin(), lines.end());
    if (max == lines.end()) {
        return {};
    }
    return *max;
}

std::optional<size_t> Source::LineToAddr(size_t addr) const {
    if (!line_mapping) {
        return {};
    }
    return line_mapping->GetAddress(addr);
}

std::vector<std::string_view> Source::GetLinesRange(size_t idx, size_t amount) const {
    if (!source_file) {
        return {};
    }
    std::vector<std::string_view> result;
    for (size_t i = 0; i < amount; ++i) {
        auto r = source_file->GetLine(i + idx);
        if (!r) {
            break;
        }
        result.emplace_back(*r);
    }
    return result;
}

std::optional<std::string_view> Source::GetLine(size_t line) const {
    if (source_file) {
        return source_file->GetLine(line);
    } else {
        return {};
    }
}

/// Returns DIE with given id or nullptr if not found.
const DIE* FindDIEById(const DIE& die, size_t id) {
    auto found_id = FindDieAttribute<ATTR_id>(die);
    if (found_id && found_id->id == id) {
        return &die;
    }

    for (const auto& child: die) {
        auto found = FindDIEById(child, id);
        // IDs are unique, we can stop if we found one.
        if (found) {
            return found;
        }
    }
    return nullptr;
}

std::optional<std::string> Source::GetFunctionNameByAddress(uint64_t address) const {
    if (!top_die) {
        return {};
    }
    // NOTE: we assume that nested functions aren't possible
    for (const auto& die: *top_die) {
        if (die.get_tag() == DIE::TAG::function) {
            log_info("Found function DIE");
            auto begin_addr = FindDieAttribute<ATTR_begin_addr>(die);
            auto end_addr = FindDieAttribute<ATTR_end_addr>(die);
            log_debug("Begin and end pointers: {} {}", fmt::ptr(begin_addr), fmt::ptr(end_addr));
            if (begin_addr && end_addr
                    && begin_addr->addr <= address && address < end_addr->addr) {
                auto name = FindDieAttribute<ATTR_name>(die);
                if (name) {
                    return name->n;
                }
            }
        }
    }
    return {};
}

std::optional<std::pair<uint64_t, uint64_t>> Source::GetFunctionAddrByName(std::string_view name) const {
    if (!top_die) {
        return {};
    }
    const auto& top_die = *this->top_die;
    // NOTE: we assume that nested functions aren't possible
    for (const auto& die: top_die) {
        if (die.get_tag() == DIE::TAG::function) {
            auto name_attr = FindDieAttribute<ATTR_name>(die);
            if (name_attr == nullptr || name_attr->n != name) {
                continue;
            }

            auto begin_addr = FindDieAttribute<ATTR_begin_addr>(die);
            if (begin_addr == nullptr) {
                return {};
            }
            auto end_addr = FindDieAttribute<ATTR_end_addr>(die);
            if (end_addr == nullptr) {
                return {};
            }
            return {std::make_pair(begin_addr->addr, end_addr->addr)};
        }
    }
    return {};
}

const DIE* Source::GetVariableDie(uint64_t address, std::string_view name,
                                  const DIE& die) const {
    auto vars = GetActiveVariables(address);
    std::string n = std::string{name};
    if (vars.contains(n)) {
        return vars.at(n);
    }
    return nullptr;
}

std::optional<expr::Location> Source::GetVariableLocation(Native& native,
                                                          std::string_view name) const {
    if (!top_die) {
        return {};
    }
    auto var = GetVariableDie(native.GetIP(), name, *top_die);
    if (var == nullptr) {
        return {};
    }

    auto location_attr = FindDieAttribute<ATTR_location_expr>(*var);
    if (location_attr == nullptr
            || location_attr->locs.empty()) {
        return {};
    }

    auto loc = ExpressionInterpreter::Interpret(location_attr->locs, native);
    return loc;
}

void Source::ReconstructTypeInformation() {
    if (!top_die) {
        return;
    }
    for (auto&& die: *top_die) {
        // Every type must have an ID.
        auto id = FindDieAttribute<ATTR_id>(die);
        if (!id) {
            continue;
        }
        if (types.contains(id->id)) {
            log_error("Type of id {} already exists! Skipping this one", id->id);
            continue;
        }
        if (die.get_tag() == DIE::TAG::primitive_type) {
            auto id = FindDieAttribute<ATTR_id>(die);
            if (!id) {
                continue;
            }
            auto name = FindDieAttribute<ATTR_name>(die);
            if (!name) {
                continue;
            }
            auto primitive_type = ToPrimitiveType(name->n);
            if (!primitive_type) {
                log_error("Unsupported primitive type '{}'", name->n);
                continue;
            }
            auto size = FindDieAttribute<ATTR_size>(die);
            if (!size) {
                log_error("Size not found");
                continue;
            }
            types[id->id] = PrimitiveType{.type = *primitive_type, .size = size->size};
        } else if (die.get_tag() == DIE::TAG::structured_type) {
            auto name = FindDieAttribute<ATTR_name>(die);
            auto size = FindDieAttribute<ATTR_size>(die);
            auto members = FindDieAttribute<ATTR_members>(die);
            std::vector<StructuredMember> members_vec;
            std::ranges::transform(members->m, std::back_inserter(members_vec),
                    [](auto &&m) {
                return StructuredMember{m.name, m.type_id, m.offset};
            });
            auto result = StructuredType{.name = name->n,
                                         .size = size->size,
                                         .members = std::move(members_vec)};
            types[id->id] = std::move(result);
        } else if (die.get_tag() == DIE::TAG::pointer_type) {
            auto pointing_to = FindDieAttribute<ATTR_type>(die);
            if (!pointing_to) {
                log_error("DIE pointer type, missing pointing attr");
                continue;
            }
            auto size = FindDieAttribute<ATTR_size>(die);
            if (!size) {
                log_error("DIE pointer_type: Missing size");
                continue;
            }
            auto ptr = PointerType{.type_id = pointing_to->type_id,
                                   .size = size->size};
            types[id->id] = ptr;
        } else if (die.get_tag() == DIE::TAG::array_type) {
            auto member_types = FindDieAttribute<ATTR_type>(die);
            if (!member_types) {
                log_error("DIE array type, missing members type");
                continue;
            }
            auto size = FindDieAttribute<ATTR_size>(die);
            if (!size) {
                log_error("DIE array type, missing number of elements");
                continue;
            }
            auto arr = ArrayType{.type_id = member_types->type_id,
                                 .cnt = size->size};
            types[id->id] = std::move(arr);
        }
        // Else not a type die.
    }
}

std::optional<Type> Source::GetVariableTypeInformation(Native& native, std::string_view name) const {
    if (!top_die) {
        return {};
    }
    auto var = GetVariableDie(native.GetIP(), name, *top_die);
    if (var == nullptr) {
        return {};
    }
    auto type = FindDieAttribute<ATTR_type>(*var);
    if (type == nullptr) {
        return {};
    }

    auto it = types.find(type->type_id);
    if (it == types.end()) {
        return {};
    }
    return types.at(type->type_id);
}

DebugEvent Source::StepIn(Native& native) const {
    // Step until the current address matches some existing line mapping
    // In case we hit an instruction level breakpoint in between stepping
    // we want to stop and report it. However, we want to step over the
    // breakpoint on current line if it is set.
    auto e = native.PerformSingleStep();
    while (std::holds_alternative<Singlestep>(e)
            && !AddrToLine(native.GetIP())) {
        e = native.DoRawSingleStep();
    }
    return e;
}

DebugEvent Source::StepOver(Native& native) const {
    // Very similar to StepIn, but the PerformStepOver itself offers
    // functionality to step over breakpoints or not.
    auto e = native.PerformStepOver();
    while (std::holds_alternative<Singlestep>(e)
            && !AddrToLine(native.GetIP())) {
        e = native.PerformStepOver(false);
    }
    return e;
}

void FindVariables(uint64_t address, const DIE& die, std::map<std::string, const DIE*>& result) {
    if (die.get_tag() == DIE::TAG::variable) {
        auto name = FindDieAttribute<ATTR_name>(die);
        if (name) {
            result.insert_or_assign(name->n, &die);
            return;
        }
    }
    if (die.get_tag() == DIE::TAG::scope ||
        die.get_tag() == DIE::TAG::function) {
        auto begin_addr = FindDieAttribute<ATTR_begin_addr>(die);
        auto end_addr = FindDieAttribute<ATTR_end_addr>(die);
        if (!begin_addr || !end_addr
                || !(begin_addr->addr <= address && address < end_addr->addr)) {
            return;
        }
    }
    for (const auto& d: die) {
        FindVariables(address, d, result);
    }
}

std::map<std::string, const DIE*> Source::GetActiveVariables(uint64_t address) const {
    std::map<std::string, const DIE*> result;
    if (top_die) {
        FindVariables(address, *top_die, result); 
    }
    return result;
}

std::pair<TypedValue, size_t>
Source::EvaluateExpression(Native& native, std::string expression, bool cache) {
    std::istringstream iss(std::move(expression));
    ExpressionParser parser(iss);
    std::unique_ptr<Expression> e; 
    try {
        e = parser.ParseExpression();
    } catch (const ParserError& err) {
        throw DebuggerError(fmt::format("Unable to parse expression: {}", err.what()));
    }
    ExpressionEvaluator eval(native, *this, evaluated_expressions);
    e->Accept(eval);
    if (cache) {
        evaluated_expressions.emplace_back(eval.YieldResult());
        return {evaluated_expressions.back(), evaluated_expressions.size() - 1};
    }
    return {eval.YieldResult(), 0};
}

std::set<std::string> Source::GetScopedVariables(uint64_t address) const {
    std::set<std::string> result;
    auto vars = GetActiveVariables(address);
    std::ranges::transform(vars, std::inserter(result, result.end()),
            [](auto&& p) { return p.first; });
    return result;
}

std::string Source::TypedValueTypeToString(const TypedValue& v) const {
    using namespace std::string_literals;
    return std::visit(utils::overloaded {
        [&](const PointerValue& v) {
            return TypeToString(v.type);
        },
        [&](const ArrayValue& v) {
            return TypeToString(v.type);
        },
        [](const CharValue&) {
            return "char"s; 
        },
        [](const IntegerValue&) {
            return "int"s; 
        },
        [](const FloatValue&) {
            return "float"s; 
        },
        [](const StructuredValue& v) {
            return v.name;
        }
    }, v);
}

std::string Source::TypeToString(const Type& type) const {
    return std::visit(utils::overloaded {
        [this](const PointerType& t) {
            if (!types.contains(t.type_id)) {
                return fmt::format("<unknown>*");
            }
            return fmt::format("{}*", TypeToString(types.at(t.type_id))); 
        },
        [this](const ArrayType& t) {
            if (!types.contains(t.type_id)) {
                return fmt::format("<unknown>*");
            }
            return fmt::format("{}[]", TypeToString(types.at(t.type_id))); 
        },
        [](const StructuredType& t) {
            return fmt::format("{}", t.name);
        },
        [](const PrimitiveType& t) {
            return FromPrimitiveType(t.type);
        },
    }, type);
}

std::string ReadMemoryAsString(Native& native, uint64_t addr) {
    std::string result;
    while (true) {
        char c = native.ReadMemory(addr++, 1).at(0);
        // TODO: Handle all unprintable or whitespace characters.
        if (c == '\0') {
            break;
        } else if (c == '\n') {
            result += "\\n";
        } else if (c == '\t') {
            result += "\\t";
        } else {
            result += c;
        }
    }
    return result;
}

std::string Source::TypedValueToString(Native& native, const TypedValue& v) {
    return std::visit(utils::overloaded {
        [&](const PointerValue& t) {
            // is const char*, print it like a string.
            auto pointee = GetType(t.type.type_id);
            if (auto pointee_type = std::get_if<PrimitiveType>(pointee);
                    pointee_type != nullptr
                    && pointee_type->type == PrimitiveType::Type::CHAR) {
                auto s = ReadMemoryAsString(native, t.value);
                return fmt::format("{} \"{}\"", t.value, s);
            } else {
                return fmt::format("{}", t.value);
            }
        },
        [&](const ArrayValue& v) {
            if (!v.members.empty()
                    && std::holds_alternative<CharValue>(v.members.back())) {
                auto s = ReadMemoryAsString(native, v.begin_address);
                return fmt::format("{} \"{}\"", v.begin_address, s);
            } else {
                std::vector<std::string> res;
                for (auto&& member: v.members) {
                    res.emplace_back(TypedValueToString(native, member));
                }
                return fmt::format("[{}]", utils::join(res.begin(), res.end(), ", "));
            }
        },
        [](const IntegerValue& v) {
            return std::to_string(v.value);
        },
        [](const FloatValue& v) {
            return std::to_string(v.value);
        },
        [](const CharValue& v) {
            return fmt::format("'{}'", v.value);
        },
        [&](const StructuredValue& v) {
            std::vector<std::string> members;
            for (auto&& member: v.members) {
                members.emplace_back(fmt::format(
                    "{} = {}", member.first,
                    TypedValueToString(native, member.second)));
            }
            auto res = utils::join(members.begin(), members.end(), ", ");
            return fmt::format("{{ {} }}", res);
        }
    }, v);
}

uint64_t Source::GetAddressFromString(std::string_view s, bool start_at_one) const {
    uint64_t address;
    auto line = utils::svtonum<uint64_t>(s);
    if (!line) {
        auto function = GetFunctionAddrByName(s);
        if (!function) {
            throw DebuggerError(fmt::format("Expected line or function name, '{}' is neither.", s));
        }
        address = function->first;
    } else {
        if (*line == 0 && start_at_one) {
            throw DebuggerError("Lines starts from one");
        }
        auto address_opt = LineToAddr(*line - start_at_one);
        if (!address_opt) {
            throw DebuggerError(fmt::format("No debug info for line {}", *line));
        }
        address = *address_opt;
    }
    return address;
}
