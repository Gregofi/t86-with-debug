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
    uint64_t SetSourceSoftwareBreakpoint(Native& native, size_t line) {
        auto mapping = UnwrapOptional(line_mapping, "No debug info for line mapping");
        auto addr =
            UnwrapOptional(mapping.GetAddress(line),
                           fmt::format("No debug info for line '{}'", line));
        native.SetBreakpoint(addr);
        return addr;
    }

    /// Removes software breakpoint at given line and returns the address
    /// of where the breakpoint was (the assembly address).
    uint64_t UnsetSourceSoftwareBreakpoint(Native& native, size_t line) {
        auto mapping = UnwrapOptional(line_mapping, "No debug info for line mapping");
        auto addr =
            UnwrapOptional(mapping.GetAddress(line),
                           fmt::format("No debug info for line '{}'", line));
        native.UnsetBreakpoint(addr);
        return addr;
    }

    uint64_t EnableSourceSoftwareBreakpoint(Native& native, size_t line) {
        auto mapping = UnwrapOptional(line_mapping, "No debug info for line mapping");
        auto addr =
            UnwrapOptional(mapping.GetAddress(line),
                           fmt::format("No debug info for line '{}'", line));
        native.EnableSoftwareBreakpoint(addr);
        return addr;
    }

    uint64_t DisableSourceSoftwareBreakpoint(Native& native, size_t line) {
        auto mapping = UnwrapOptional(line_mapping, "No debug info for line mapping");
        auto addr =
            UnwrapOptional(mapping.GetAddress(line),
                           fmt::format("No debug info for line '{}'", line));
        native.DisableSoftwareBreakpoint(addr);
        return addr;
    }

    /// Returns latest line that corresponds to given address if 
    /// debugging information is available, otherwise returns nullopt.
    std::optional<size_t> AddrToLine(size_t addr) {
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

    std::optional<size_t> LineToAddr(size_t addr) {
        if (!line_mapping) {
            return {};
        }
        return line_mapping->GetAddress(addr);
    }

    /// Returns lines from the source file. This function does
    /// not throw if out of bounds, instead it stops.
    /// For example if the program has two lines,
    /// and idx=1, amount=3, it would return the second line only.
    /// Returns empty vector if no debugging info is provided.
    std::vector<std::string_view> GetLines(size_t idx, size_t amount) const {
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

    std::optional<std::string_view> GetLine(size_t line) {
        return source_file->GetLine(line);
    }
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
