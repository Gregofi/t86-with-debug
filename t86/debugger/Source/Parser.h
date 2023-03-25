#pragma once
#include <iostream>
#include <map>
#include <fmt/core.h>
#include "common/parsing.h"
#include "debugger/Source/Die.h"
#include "debugger/Source/SourceFile.h"

namespace dbg {
struct DebuggingInfo {
    std::optional<std::map<size_t, uint64_t>> line_mapping;
    std::optional<DIE> top_die;
    std::optional<SourceFile> source_code;
};

/// Parser debugging information, like DIEs, line to addr mapping and so on.
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
    DIE DebugInfo();
    DIE ParseDIE(std::string name);
    ATTR_members StructuredMembers();
    DIE::TAG ParseDIETag(std::string_view v) const;
    expr::Offset ParseOffset();
    DIE_ATTR ParseATTR(std::string_view v);
    std::vector<expr::LocExpr> ParseExprLoc();
    expr::Location ParseOperand();
    expr::LocExpr ParseOneExprLoc();
    TokenKind GetNext();

    Lexer lex;
    Token curtok;
};
}
