#include "debugger/Source/Source.h"
#include "debugger/Source/ExpressionInterpreter.h"
#include "debugger/Source/LineMapping.h"

uint64_t Source::SetSourceSoftwareBreakpoint(Native& native, size_t line) {
    auto mapping = UnwrapOptional(line_mapping, "No debug info for line mapping");
    auto addr =
        UnwrapOptional(mapping.GetAddress(line),
                       fmt::format("No debug info for line '{}'", line));
    native.SetBreakpoint(addr);
    return addr;
}

uint64_t Source::UnsetSourceSoftwareBreakpoint(Native& native, size_t line) {
    auto mapping = UnwrapOptional(line_mapping, "No debug info for line mapping");
    auto addr =
        UnwrapOptional(mapping.GetAddress(line),
                       fmt::format("No debug info for line '{}'", line));
    native.UnsetBreakpoint(addr);
    return addr;
}

uint64_t Source::EnableSourceSoftwareBreakpoint(Native& native, size_t line) {
    auto mapping = UnwrapOptional(line_mapping, "No debug info for line mapping");
    auto addr =
        UnwrapOptional(mapping.GetAddress(line),
                       fmt::format("No debug info for line '{}'", line));
    native.EnableSoftwareBreakpoint(addr);
    return addr;
}

uint64_t Source::DisableSourceSoftwareBreakpoint(Native& native, size_t line) {
    auto mapping = UnwrapOptional(line_mapping, "No debug info for line mapping");
    auto addr =
        UnwrapOptional(mapping.GetAddress(line),
                       fmt::format("No debug info for line '{}'", line));
    native.DisableSoftwareBreakpoint(addr);
    return addr;
}

std::optional<size_t> Source::AddrToLine(size_t addr) {
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

std::optional<size_t> Source::LineToAddr(size_t addr) {
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

std::optional<std::string_view> Source::GetLine(size_t line) {
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

std::optional<uint64_t> Source::GetAddrFunctionByName(std::string_view name) {
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
            return {begin_addr->addr};
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
                                                          std::string_view name) {
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
