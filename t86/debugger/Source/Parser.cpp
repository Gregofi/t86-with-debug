#include "Parser.h"
#include "helpers.h"
#include "logger.h"
#include "common/parsing.h"

namespace dbg {

TokenKind Parser::GetNext() {
    curtok = lex.getNext();
    return curtok.kind;
}

std::map<size_t, uint64_t> Parser::DebugLine() {
    std::map<size_t, uint64_t> location_mapping;
    while (curtok.kind != TokenKind::DOT && curtok.kind != TokenKind::END) {
        if (curtok.kind != TokenKind::NUM) {
            throw CreateError("Expected line entry in form 'row:col'");
        }
        auto source_line = lex.getNumber();
        if (GetNext() != TokenKind::DOUBLEDOT) {
            throw CreateError("Expected line entry in form 'row:col'");
        }
        if (GetNext() != TokenKind::NUM) {
            throw CreateError("Expected line entry in form 'row:col'");
        }
        auto address = lex.getNumber();
        location_mapping[source_line] = address;
        GetNext();
    } 
    return location_mapping;
}

DebuggingInfo Parser::Parse() {
    DebuggingInfo info;
    while (curtok.kind != TokenKind::END) {
        if (curtok.kind != TokenKind::DOT) {
            throw CreateError("Expected section beginning with '.'");
        }
        GetNext();
        if (curtok.kind != TokenKind::ID) {
            throw CreateError("Expected section name");
        }
        auto section_name = lex.getId();
        GetNext();

        if (section_name == "debug_line") {
            info.line_mapping = DebugLine();
        } else {
            log_info("Skipping section '{}'", section_name);
            while (curtok.kind != TokenKind::DOT && curtok.kind != TokenKind::END) {
                GetNext();
            }
        }
    }
    return info;
}

}
