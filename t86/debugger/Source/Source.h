#pragma once
#include <algorithm>
#include "debugger/Source/LineMapping.h"
#include "debugger/Source/SourceFile.h"
#include "debugger/Source/Type.h"
#include "debugger/Native.h"
#include "debugger/Source/Die.h"

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

    /// Sets software breakpoint at given line and returns the address
    /// of the breakpoint (the assembly address).
    uint64_t SetSourceSoftwareBreakpoint(Native& native, size_t line) const;

    /// Removes software breakpoint at given line and returns the address
    /// of where the breakpoint was (the assembly address).
    uint64_t UnsetSourceSoftwareBreakpoint(Native& native, size_t line) const;
    uint64_t EnableSourceSoftwareBreakpoint(Native& native, size_t line) const;

    uint64_t DisableSourceSoftwareBreakpoint(Native& native, size_t line) const;
    /// Returns function that owns instruction at given address.
    std::optional<std::string> GetFunctionNameByAddress(uint64_t address) const;
    /// Returns the address of the function prologue.
    std::optional<uint64_t> GetAddrFunctionByName(std::string_view name) const;
    /// Returns the type of a variable if it is in current scope.
    std::optional<Type> GetVariableTypeInformation(Native& native, std::string_view name) const;
    /// Returns a location of variable.
    /// Be aware that this can make a lot of calls to the underlying debugged
    /// process, depending on how complicated is the location expression.
    std::optional<expr::Location> GetVariableLocation(Native& native, std::string_view name) const;

    /// Returns latest line that corresponds to given address if 
    /// debugging information is available, otherwise returns nullopt.
    std::optional<size_t> AddrToLine(size_t addr) const;

    std::optional<size_t> LineToAddr(size_t addr) const;

    /// Returns lines from the source file. This function does
    /// not throw if out of bounds, instead it stops.
    /// For example if the program has two lines,
    /// and idx=1, amount=3, it would return the second line only.
    /// Returns empty vector if no debugging info is provided.
    std::vector<std::string_view> GetLines(size_t idx, size_t amount) const;

    std::optional<std::string_view> GetLine(size_t line) const;
private:
    std::optional<Type> ReconstructTypeInformation(size_t id) const;

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
};
