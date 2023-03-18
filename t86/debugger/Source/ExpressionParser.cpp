#include "ExpressionParser.h"

std::unique_ptr<Expression> ExpressionParser::expr() {
    return equality(); 
}

std::unique_ptr<Expression> ExpressionParser::equality() {
    std::unique_ptr<Expression> result = comparison();
    while (curtok.kind == TokenKind::EQ) {
        GetNext();
        auto next = comparison();
        NOT_IMPLEMENTED;
    }
    return result;
}

std::unique_ptr<Expression> ExpressionParser::comparison() {
    std::unique_ptr<Expression> result = term();
    while (curtok.kind == TokenKind::LESS || curtok.kind == TokenKind::GREATER) {
        GetNext();
        auto next = factor();
        NOT_IMPLEMENTED;
    }
    return result;
}

std::unique_ptr<Expression> ExpressionParser::term() {
    std::unique_ptr<Expression> result = factor();
    while (curtok.kind == TokenKind::PLUS) {
        GetNext();
        auto next = factor();
        result = std::make_unique<Plus>(std::move(result), std::move(next));
    }
    return result;
}

std::unique_ptr<Expression> ExpressionParser::factor() {
    std::unique_ptr<Expression> result = unary();
    while (curtok.kind == TokenKind::TIMES) {
        GetNext();
        auto next = unary();
        NOT_IMPLEMENTED;
    }
    return result;
}

std::unique_ptr<Expression> ExpressionParser::unary() {
    if (curtok.kind == TokenKind::TIMES) {
        GetNext();
        auto pstfx = postfix();
        return std::make_unique<Dereference>(std::move(pstfx));
    }
    return postfix();
}

std::unique_ptr<Expression> ExpressionParser::postfix() {
    auto p = primary();
    while (curtok.kind == TokenKind::LBRACKET
            || curtok.kind == TokenKind::DOT
            || curtok.kind == TokenKind::ARROW) {
        if (curtok.kind == TokenKind::LBRACKET) {
            GetNext();
            auto e = expr();
            if (curtok.kind != TokenKind::RBRACKET) {
                throw CreateError("Expected closing ']'");
            }
            GetNext();
            p = std::make_unique<ArrayAccess>(std::move(p), std::move(e));
        } else if (curtok.kind == TokenKind::DOT) {
            if (GetNext() != TokenKind::ID) {
                throw CreateError("Expected member name after '.'");
            }
            auto member = lex.getId();
            GetNext();
            p = std::make_unique<MemberAccess>(std::move(p), std::move(member));
        } else if (curtok.kind == TokenKind::ARROW) {
            if (GetNext() != TokenKind::ID) {
                throw CreateError("Expected member name after '.'");
            }
            auto member = lex.getId();
            GetNext();
            p = std::make_unique<MemberDereferenceAccess>(std::move(p),
                                                          std::move(member));
        } else {
            break;
        }
    }
    return p;
}

std::unique_ptr<Expression> ExpressionParser::primary() {
    if (curtok.kind == TokenKind::NUM) {
        auto num = lex.getNumber();
        GetNext();
        return std::make_unique<Integer>(num);
    } else if (curtok.kind == TokenKind::FLOAT) {
        auto num = lex.getFloat();
        GetNext();
        return std::make_unique<Float>(num);
    } else if (curtok.kind == TokenKind::ID) {
        auto id = lex.getId();
        GetNext();
        return std::make_unique<Identifier>(std::move(id));
    } else if (curtok.kind == TokenKind::LPAREN) {
        GetNext();
        auto e = expr();
        if (curtok.kind != TokenKind::RPAREN) {
            throw CreateError("Expected closing parentheses");
        }
        GetNext();
        return e;
    } else {
        throw CreateError("Expected either identifier, int or float");
    }
}

