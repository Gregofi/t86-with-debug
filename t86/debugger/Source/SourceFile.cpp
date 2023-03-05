#include "debugger/Source/SourceFile.h"

std::optional<std::string_view> SourceFile::GetLine(size_t idx) const {
    if (idx >= lines.size()) {
        return {};
    }
    return lines[idx];
}
