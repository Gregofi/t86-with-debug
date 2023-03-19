#pragma once
#include <algorithm>
#include "debugger/Source/Expression.h"
#include "debugger/Source/ExpressionParser.h"
#include "debugger/Source/LineMapping.h"
#include "debugger/Source/SourceFile.h"
#include "debugger/Source/Type.h"
#include "debugger/Native.h"
#include "debugger/Source/Die.h"

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

/// Responsible for all source level debugging.
class Source {
public:
    void RegisterSourceFile(SourceFile file) {
        source_file = std::move(file);
    }

    void RegisterLineMapping(LineMapping mapping) {
        line_mapping = std::move(mapping);
    }

    void RegisterDebuggingInformation(DIE topmost_die) {
        top_die = std::move(topmost_die);
    }

    // Those four guys are currently unused but are left here
    // if needed in the future.
    /// Sets software breakpoint at given line and returns the address
    /// of the breakpoint (the assembly address).
    uint64_t SetSourceSoftwareBreakpoint(Native& native, size_t line) const;
    /// Removes software breakpoint at given line and returns the address
    /// of where the breakpoint was (the assembly address).
    uint64_t UnsetSourceSoftwareBreakpoint(Native& native, size_t line) const;
    uint64_t EnableSourceSoftwareBreakpoint(Native& native, size_t line) const;
    uint64_t DisableSourceSoftwareBreakpoint(Native& native, size_t line) const;

    /// Tries to parse the 's' as number, if it succeeds then
    /// returns an address that corresponds to line represented by 's'.
    /// If the parse fails then the 's' is interpreted as function name
    /// and the beginning address of that function is returned.
    /// Throws if the information cannot be retrieved.
    uint64_t GetAddressFromString(std::string_view s) {
        uint64_t address;
        auto line = utils::svtonum<uint64_t>(s);
        if (!line) {
            auto function = GetFunctionAddrByName(s);
            if (!function) {
                throw DebuggerError(fmt::format("Expected line or function name, '{}' is neither.", s));
            }
            address = function->first;
        } else {
            auto address_opt = LineToAddr(*line);
            if (!address_opt) {
                throw DebuggerError(fmt::format("No debug info for line {}", *line));
            }
            address = *address_opt;
        }
        return address;
    }

    /// Returns function that owns instruction at given address.
    std::optional<std::string> GetFunctionNameByAddress(uint64_t address) const;
    /// Returns the address of the function prologue.
    std::optional<std::pair<uint64_t, uint64_t>> GetFunctionAddrByName(std::string_view name) const;
    /// Returns the address of the last functions instruction plus one.
    std::optional<Type> GetVariableTypeInformation(Native& native, std::string_view name) const;
    /// Returns a location of variable.
    /// Be aware that this can make a lot of calls to the underlying debugged
    /// process, depending on how complicated is the location expression.
    std::optional<expr::Location> GetVariableLocation(Native& native, std::string_view name) const;

    /// Returns latest line that corresponds to given address if 
    /// debugging information is available, otherwise returns nullopt.
    std::optional<size_t> AddrToLine(size_t addr) const;

    std::optional<size_t> LineToAddr(size_t addr) const;

    /// Return names of variables that are currently in scope.
    std::set<std::string> GetScopedVariables(uint64_t address) const;

    TypedValue EvaluateExpression(Native& native, std::string expression);

    /// Returns lines from the source file. This function does
    /// not throw if out of bounds, instead it stops.
    /// For example if the program has two lines,
    /// and idx=1, amount=3, it would return the second line only.
    /// Returns empty vector if no debugging info is provided.
    std::vector<std::string_view> GetLines(size_t idx, size_t amount) const;

    std::optional<std::string_view> GetLine(size_t line) const;

    /// Performs a source level step in.
    /// Beware, line number information should be complete if you
    /// wish to use this, otherwise weird behaviour might occur,
    /// like skipping parts of functions etc.
    DebugEvent StepIn(Native& native) const;

    /// Performs a source level step over.
    /// This works on line information, if it is wrong or missing
    /// this will behave seemingly awkwardly.
    DebugEvent StepOver(Native& native) const;

    std::optional<Type> ReconstructTypeInformation(size_t id) const;
private:
    friend class ExpressionEvaluator;
    std::map<std::string, const DIE*> GetActiveVariables(uint64_t address) const;

    template<typename T>
    static const T& UnwrapOptional(const std::optional<T>& opt, const std::string& message) {
        if (!opt) {
            throw DebuggerError(message);
        }
        return *opt;
    }

    /// Finds the innermost variable die in the provided die,
    /// ie. the most nested one in scope (doesn't search through nested
    /// functions). If the DIE is not found then nullptr is returned.
    const DIE* GetVariableDie(uint64_t address, std::string_view name,
                              const DIE& die) const;
    std::optional<LineMapping> line_mapping;
    std::optional<SourceFile> source_file;
    std::optional<DIE> top_die;
    mutable std::map<size_t, Type> cached_types;
};
