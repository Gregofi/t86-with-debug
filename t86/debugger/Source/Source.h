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

    void SetSourceSoftwareBreakpoint(Native& native, size_t line) {
        auto mapping = UnwrapOptional(line_mapping, "No debug info for line mapping");
        auto addr = mapping.GetAddress(line);
        if (!addr) {
            throw DebuggerError(fmt::format("No debug info for line '{}'", line));
        }
        native.SetBreakpoint(*addr);
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
    /// not throw if out of bounds, instead it ignores out of
    /// bound access. For example if the program has two lines,
    /// and idx=1, amount=3, it would return the second line only.
    /// Returns empty vector if no debugging info is provided.
    std::vector<std::string_view> GetLines(size_t idx, size_t amount) {
        if (!source_file) {
            return {};
        }
        std::vector<std::string_view> result;
        for (size_t i = 0; i < amount; ++i) {
            auto r = source_file->GetLine(i + idx);
            if (r) {
                result.emplace_back(*r);
            }
        }
        return result;
    }
private:
    template<typename T>
    static T& UnwrapOptional(std::optional<T>& opt, const std::string& message) {
        if (!opt) {
            throw DebuggerError(message);
        }
        return *opt;
    }

    std::optional<LineMapping> line_mapping;
    std::optional<SourceFile> source_file;
};
