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

std::vector<std::string_view> Source::GetLines(size_t idx, size_t amount) const {
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
    return source_file->GetLine(line);
}

/// Finds given attribute in a DIE and returns a pointer to it,
/// if the attribute is not found a nullptr is returned.
template<typename Attr>
const Attr* FindDieAttribute(const DIE& die) {
    for (auto it = die.begin_attr(); it != die.end_attr(); ++it) {
        auto found = std::visit([](auto&& arg) -> const Attr* {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<Attr, T>) {
                return &arg;
            } else {
                return nullptr;
            }
        }, *it);
        if (found != nullptr) {
            return found;
        }
    }
    return nullptr;
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
    auto top_die = UnwrapOptional(this->top_die, "No debugging information provided");
    // NOTE: we assume that nested functions aren't possible
    for (const auto& die: top_die) {
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
    auto top_die = UnwrapOptional(this->top_die, "No debugging information provided");
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
    if (die.get_tag() == DIE::TAG::variable) {
        auto name_attr = FindDieAttribute<ATTR_name>(die);
        if (name_attr->n == name) {
            return &die;
        } else {
            return nullptr;
        }
    }

    // Check that we are inside scope
    if (die.get_tag() == DIE::TAG::scope ||
        die.get_tag() == DIE::TAG::function) {
        auto begin_addr = FindDieAttribute<ATTR_begin_addr>(die);
        auto end_addr = FindDieAttribute<ATTR_end_addr>(die);
        if (!begin_addr || !end_addr
                || !(begin_addr->addr <= address && address < end_addr->addr)) {
            return nullptr;
        }
    }
    const DIE* variable = nullptr;
    for (const auto& d: die) {
        auto found_var = GetVariableDie(address, name, d);
        // Overwrite old variable with newer, because newer is more
        // nested, ie. it overshadows the old one.
        if (found_var != nullptr) {
            variable = found_var;
        }
    }
    return variable;
}

std::optional<expr::Location> Source::GetVariableLocation(Native& native,
                                                          std::string_view name) const {
    auto top_die = UnwrapOptional(this->top_die, "No debugging information provided");
    auto var = GetVariableDie(native.GetIP(), name, top_die);
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

std::optional<Type> Source::ReconstructTypeInformation(size_t id) const {
    auto type_die = FindDIEById(*top_die, id);
    if (type_die == nullptr) {
        return {};
    }
    if (type_die->get_tag() == DIE::TAG::primitive_type) {
        auto name = FindDieAttribute<ATTR_name>(*type_die);
        if (!name) {
            return {};
        }
        auto primitive_type = ToPrimitiveType(name->n);
        if (!primitive_type) {
            log_info("DIE id {}: Unsupported primitive type '{}'", id, name->n);
            return {};
        }
        auto size = FindDieAttribute<ATTR_size>(*type_die);
        if (!size) {
            log_info("DIE id {}: Size not found", id);
            return {};
        }
        return PrimitiveType{.type = *primitive_type, .size = size->size};
    } else if (type_die->get_tag() == DIE::TAG::structured_type) {
        auto name = FindDieAttribute<ATTR_name>(*type_die);
        if (!name) {
            return {};
        }
        auto size = FindDieAttribute<ATTR_size>(*type_die);
        if (!size) {
            return StructuredType{.name = name->n, .size = 0};
        }
        auto members = FindDieAttribute<ATTR_members>(*type_die);
        if (!members) {
            return StructuredType{.name = name->n, .size = size->size};
        }
        std::vector<std::pair<int64_t, std::optional<Type>>> members_vec;
        std::ranges::transform(members->m, std::back_inserter(members_vec),
                [this](auto &&m) {
            return std::make_pair(m.first, ReconstructTypeInformation(m.second));
        });
        return StructuredType{.name = name->n, .size = size->size,
                              .members = std::move(members_vec)};
    } else if (type_die->get_tag() == DIE::TAG::pointer_type) {
        auto pointing_to = FindDieAttribute<ATTR_type>(*type_die);
        if (!pointing_to) {
            log_info("DIE pointer type, missing pointing attr");
            return PointerType{.size = 0};
        }
        auto size = FindDieAttribute<ATTR_size>(*type_die);
        auto pointed_die = FindDIEById(*top_die, pointing_to->type_id);
        if (!pointing_to || !size || !pointed_die) {
            log_info("DIE pointer_type, id {}: Missing either dest or size", id);
            return PointerType{.size = 0};
        }
        // We can't just recursively call ourselves because that might
        // lead to infinite recursion, since structs can point to each other.
        if (pointed_die->get_tag() == DIE::TAG::structured_type) {
            auto name = FindDieAttribute<ATTR_name>(*pointed_die);
            if (name) {
                auto structured_type
                    = std::make_unique<Type>(StructuredType{.name = name->n});
                return PointerType{.to = std::move(structured_type),
                                   .size = size->size};
            }
        } else {
            auto pointed = ReconstructTypeInformation(pointing_to->type_id);
            if (pointed) {
                return PointerType{.to = std::make_shared<Type>(std::move(*pointed)),
                                   .size = size->size};
            } else {
                return PointerType{.size = size->size};
            }
        }
    } else {
        log_error("Unknown DIE tag describing type");
        UNREACHABLE;
    }
    UNREACHABLE;
}

std::optional<Type> Source::GetVariableTypeInformation(Native& native,
                                                       std::string_view name) const {
    auto top_die = UnwrapOptional(this->top_die, "No debugging information provided!");
    auto var = GetVariableDie(native.GetIP(), name, top_die);
    if (var == nullptr) {
        return {};
    }
    auto type = FindDieAttribute<ATTR_type>(*var);
    if (type == nullptr) {
        return {};
    }
    
    return ReconstructTypeInformation(type->type_id);
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
