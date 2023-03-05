#pragma once
#include <iostream>
#include <map>
#include <fmt/core.h>
#include "common/parsing.h"

namespace dbg {
struct DebuggingInfo {
    std::optional<std::map<size_t, uint64_t>> line_mapping;
};

class Parser {
public:
    Parser(std::istream& iss) noexcept : lex(iss) { GetNext(); }

    template<typename ...Args>
    ParserError CreateError(fmt::format_string<Args...> format, Args&& ...args) const {
        return ParserError(fmt::format("Error:{}:{}:{}", curtok.row,
                    curtok.col, fmt::format(format,
                        std::forward<Args>(args)...)));
    }

    DebuggingInfo Parse();
private:
    std::map<size_t, uint64_t> DebugLine();
    void DebugMetadata();
    TokenKind GetNext();
    Lexer lex;
    Token curtok;
};
}
