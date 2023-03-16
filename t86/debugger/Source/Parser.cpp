#include <string>
#include "Parser.h"
#include "debugger/Source/Die.h"
#include "helpers.h"
#include "logger.h"
#include "common/parsing.h"

namespace dbg {

TokenKind Parser::GetNext() {
    curtok = lex.getNext();
    return curtok.kind;
}

std::map<int64_t, size_t> Parser::StructuredMembers() {
    std::map<int64_t, size_t> res;
    while (curtok.kind != TokenKind::RBRACE) {
        if (curtok.kind != TokenKind::NUM) {
            throw CreateError("Expected entry in form 'offset:type_id'");
        }
        auto offset = lex.getNumber();
        if (GetNext() != TokenKind::DOUBLEDOT) {
            throw CreateError("Expected entry in form 'offset:type_id'");
        }
        if (GetNext() != TokenKind::NUM) {
            throw CreateError("Expected entry in form 'offset:type_id'");
        }
        auto type = lex.getNumber();
        res[offset] = type;
        if (GetNext() == TokenKind::COMMA) {
            GetNext();
        } else if (curtok.kind != TokenKind::RBRACE) {
            throw CreateError("Expected comma or closing brace");
        }
    }
    GetNext();
    return res;
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

DIE::TAG Parser::ParseDIETag(std::string_view v) const {
    if (v == "DIE_function") {
        return DIE::TAG::function;
    } else if (v == "DIE_primitive_type") {
        return DIE::TAG::primitive_type;
    } else if (v == "DIE_structured_type") {
        return DIE::TAG::structured_type;
    } else if (v == "DIE_pointer_type") {
        return DIE::TAG::pointer_type;
    } else if (v == "DIE_variable") {
        return DIE::TAG::variable;
    } else if (v == "DIE_scope") {
        return DIE::TAG::scope;
    } else if (v == "DIE_compilation_unit") {
        return DIE::TAG::compilation_unit;
    } else {
        throw CreateError("Unknown DIE tag '{}'.", v);
    }
}

expr::Location Parser::ParseOperand() {
    if (curtok.kind == TokenKind::NUM) {
        auto num = lex.getNumber();
        GetNext();
        return expr::Offset{num};
    } else if (curtok.kind == TokenKind::ID) {
        // Do not sanitize register names here
        // because this information should be
        // architecture independent.
        auto id = lex.getId();
        GetNext();
        return expr::Register{id};
    } else {
        throw CreateError("Unexpected token");
    }
}

expr::LocExpr Parser::ParseOneExprLoc() {
    if (curtok.kind == TokenKind::ID) {
        auto id = lex.getId();
        GetNext();
        if (id == "BASE_REG_OFFSET") {
            if (curtok.kind == TokenKind::NUM) {
                auto num = lex.getNumber();
                GetNext();
                return expr::FrameBaseRegisterOffset{num};
            } else {
                throw CreateError("BASE_REG_OFFSET instruction can only have number as its operand");
            }
        } else if (id == "PUSH") {
            auto operand = ParseOperand(); 
            return expr::Push{std::move(operand)};
        } else if (id == "ADD") {
            return expr::Add{};
        } else {
            throw CreateError("Unknown instruction '{}'", id);
        }
    } else {
        throw CreateError("Unexpected token when parsing expression location");
    }
}

std::vector<expr::LocExpr> Parser::ParseExprLoc() {
    std::vector<expr::LocExpr> result;
    if (curtok.kind == TokenKind::BACKTICK) {
        if (GetNext() != TokenKind::BACKTICK) {
            auto loc = ParseOneExprLoc();
            result.push_back(std::move(loc));
        }
        GetNext(); // Eat the '`'
    } else if (curtok.kind == TokenKind::LBRACKET) {
        GetNext();
        while (curtok.kind != TokenKind::RBRACKET) {
            auto loc = ParseOneExprLoc();
            result.push_back(std::move(loc));
            if (curtok.kind == TokenKind::SEMICOLON) {
                GetNext(); // Eat the ';'
            } else if (curtok.kind != TokenKind::RBRACKET) {
                throw CreateError("Expected semicolon to separate expressions");
            }
        }
        GetNext();
    } else {
        throw CreateError("Expected either ` or [ as beginning of location lists");
    }
    return result;
}

DIE_ATTR Parser::ParseATTR(std::string_view v) {
    if (curtok.kind != TokenKind::DOUBLEDOT) {
        throw CreateError("Expected ':' after attribute name");
    }
    GetNext();
    if (v == "ATTR_name") {
        if (curtok.kind != TokenKind::ID && curtok.kind != TokenKind::STRING) {
            throw CreateError("ATTR_name should have a string as its value");
        }
        std::string id;
        if (curtok.kind == TokenKind::ID) {
            id = lex.getId();
        } else {
            id = lex.getStr();
        }
        GetNext();
        return ATTR_name{std::move(id)};
    } else if (v == "ATTR_type") {
        if (curtok.kind != TokenKind::NUM) {
            throw CreateError("ATTR_type should have a number as its value");
        }
        auto id = static_cast<size_t>(lex.getNumber());
        GetNext();
        return ATTR_type{std::move(id)};
    } else if (v == "ATTR_id") {
        if (curtok.kind != TokenKind::NUM) {
            throw CreateError("ATTR_id should have a number as its value");
        }
        auto id = static_cast<size_t>(lex.getNumber());
        GetNext();
        return ATTR_id{std::move(id)};
    } else if (v == "ATTR_begin_addr") {
        if (curtok.kind != TokenKind::NUM) {
            throw CreateError("ATTR_begin_addr should have a number as its value");
        }
        auto num = lex.getNumber();
        GetNext();
        return ATTR_begin_addr{static_cast<uint64_t>(num)};
    } else if (v == "ATTR_end_addr") {
        if (curtok.kind != TokenKind::NUM) {
            throw CreateError("ATTR_end_addr should have a number as its value");
        }
        auto num = lex.getNumber();
        GetNext();
        return ATTR_end_addr{static_cast<uint64_t>(num)};
    } else if (v == "ATTR_size") {
        if (curtok.kind != TokenKind::NUM) {
            throw CreateError("ATTR_end_addr should have a number as its value");
        }
        auto num = lex.getNumber();
        GetNext();
        return ATTR_size{static_cast<uint64_t>(num)};
    } else if (v == "ATTR_members") {
        if (curtok.kind != TokenKind::LBRACE) {
            throw CreateError("Expected opening brace");
        }
        GetNext();
        auto members = StructuredMembers();
        return ATTR_members{std::move(members)};
    } else if (v == "ATTR_location") {
        auto loc = ParseExprLoc(); 
        return ATTR_location_expr{std::move(loc)};
    } else {
        throw CreateError("Unknown DIE attribute '{}'", v);
    }
}

DIE Parser::ParseDIE(std::string name) {
    if (curtok.kind != TokenKind::DOUBLEDOT) {
        throw CreateError("Expected ':' after DIE name");
    }
    if (GetNext() != TokenKind::LBRACE) {
        throw CreateError("Expected left brace");
    }
    GetNext();
    DIE::TAG tag = ParseDIETag(name);
    std::vector<DIE_ATTR> attributes;
    std::vector<DIE> childs;
    while (curtok.kind != TokenKind::RBRACE) {
        auto id = lex.getId();
        GetNext();
        if (id.starts_with("ATTR")) {
            auto attr = ParseATTR(std::move(id));
            attributes.push_back(std::move(attr));
        } else if (id.starts_with("DIE")) {
            auto die = ParseDIE(std::move(id));
            childs.push_back(std::move(die));
        }
        if (curtok.kind == TokenKind::COMMA) {
            GetNext();
        } else if (curtok.kind != TokenKind::RBRACE) {
            throw CreateError("Expected either comma or closing brace");
        }
    }
    GetNext(); // Eat }
    return DIE(tag, std::move(attributes), std::move(childs));
}

DIE Parser::DebugInfo() {
    if (curtok.kind != TokenKind::DOT
            && curtok.kind != TokenKind::END) {
        if (curtok.kind != TokenKind::ID) {
            throw CreateError("Expected DIE tag name");
        }
        auto id = lex.getId();
        GetNext();
        auto topmost_die = ParseDIE(std::move(id));
        return topmost_die;
    } else {
        // Empty DIE
        return {DIE::TAG::invalid, {} , {}};
    }
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
        // This must be done before fetching next token, otherwise
        // we will lose the beginning of the string to tokenization.
        if (section_name == "debug_source") {
            std::string source_code = lex.RawMode();
            // Since the raw mode eats everything after the .debug_source header,
            // we need to get rid of the newline.
            info.source_code = SourceFile(source_code);
            // debug_source must be last section
            return info;
        }
        GetNext();

        if (section_name == "debug_line") {
            info.line_mapping = DebugLine();
        } else if (section_name == "debug_info") {
            info.top_die = DebugInfo();
        } else {
            lex.SetIgnoreMode(true);
            log_info("Skipping section '{}'", section_name);
            while (curtok.kind != TokenKind::DOT && curtok.kind != TokenKind::END) {
                GetNext();
            }
            lex.SetIgnoreMode(false);
        }
    }
    return info;
}
}
