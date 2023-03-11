#pragma once
#include <algorithm>
#include "debugger/Source/LineMapping.h"
#include "debugger/Source/SourceFile.h"
#include "debugger/Native.h"

/// Responsible for all source level debugging.
class Source {
public:
    void RegisterSourceFile(SourceFile file) {
        source_file = std::move(file);
    }

    void RegisterLineMapping(LineMapping mapping) {
        line_mapping = std::move(mapping);
    }

    /// Sets software breakpoint at given line and returns the address
    /// of the breakpoint (the assembly address).
    uint64_t SetSourceSoftwareBreakpoint(Native& native, size_t line);

    /// Removes software breakpoint at given line and returns the address
    /// of where the breakpoint was (the assembly address).
    uint64_t UnsetSourceSoftwareBreakpoint(Native& native, size_t line);
    uint64_t EnableSourceSoftwareBreakpoint(Native& native, size_t line);

    uint64_t DisableSourceSoftwareBreakpoint(Native& native, size_t line);

    /// Returns latest line that corresponds to given address if 
    /// debugging information is available, otherwise returns nullopt.
    std::optional<size_t> AddrToLine(size_t addr);

    std::optional<size_t> LineToAddr(size_t addr);

    /// Returns lines from the source file. This function does
    /// not throw if out of bounds, instead it stops.
    /// For example if the program has two lines,
    /// and idx=1, amount=3, it would return the second line only.
    /// Returns empty vector if no debugging info is provided.
    std::vector<std::string_view> GetLines(size_t idx, size_t amount) const;

    std::optional<std::string_view> GetLine(size_t line);
private:
    template<typename T>
    static const T& UnwrapOptional(const std::optional<T>& opt, const std::string& message) {
        if (!opt) {
            throw DebuggerError(message);
        }
        return *opt;
    }

    std::optional<LineMapping> line_mapping;
    std::optional<SourceFile> source_file;
};
