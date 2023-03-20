#pragma once
#include "common/helpers.h"

/// Manages the source file.
class SourceFile {
public:
    SourceFile(const std::string& file_content) {
        std::string_view s = file_content;
        while (true) {
            auto offset = s.find('\n');
            if (offset == s.npos) {
                if (s != "") {
                    lines.emplace_back(s);
                }
                break;
            } else {
                lines.emplace_back(s.substr(0, offset));
            }
            s.remove_prefix(offset + 1);
        }
    }
    std::vector<std::string> GetRange(size_t begin, size_t amount);
    std::optional<std::string_view> GetLine(size_t idx) const;
    const std::vector<std::string>& GetLines() const { return lines; }
private:
    std::vector<std::string> lines;
};
