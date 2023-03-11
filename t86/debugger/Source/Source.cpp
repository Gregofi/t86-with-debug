#include "debugger/Source/Source.h"
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
