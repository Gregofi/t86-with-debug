#pragma once
#include "common/helpers.h"

/// Manages the source file.
class SourceFile {
public:
    SourceFile(const std::string& file_content) {
        lines = utils::split(file_content, '\n');
    }
    std::vector<std::string> GetRange(size_t begin, size_t amount);
    std::optional<std::string_view> GetLine(size_t idx) const;
private:
    std::vector<std::string> lines;
};
