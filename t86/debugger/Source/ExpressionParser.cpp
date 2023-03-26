#include "ExpressionParser.h"

static const std::map<TokenKind, BinaryOperator::Op> operators = {
    {TokenKind::PLUS, BinaryOperator::Op::Add},
    {TokenKind::MINUS, BinaryOperator::Op::Sub},
    {TokenKind::TIMES, BinaryOperator::Op::Mul},
    {TokenKind::SLASH, BinaryOperator::Op::Div},
    {TokenKind::MOD, BinaryOperator::Op::Mod},
    {TokenKind::EQ, BinaryOperator::Op::Eq},
    {TokenKind::NEQ, BinaryOperator::Op::Neq},
    {TokenKind::LESS, BinaryOperator::Op::Less},
    {TokenKind::GREATER, BinaryOperator::Op::Greater},
    {TokenKind::GEQ, BinaryOperator::Op::Geq},
    {TokenKind::LEQ, BinaryOperator::Op::Leq},
    {TokenKind::LAND, BinaryOperator::Op::And},
    {TokenKind::LOR, BinaryOperator::Op::Or},
    {TokenKind::AND, BinaryOperator::Op::IAnd},
    {TokenKind::OR, BinaryOperator::Op::IOr},
    {TokenKind::XOR, BinaryOperator::Op::IXor},
    {TokenKind::LSHIFT, BinaryOperator::Op::LShift},
    {TokenKind::RSHIFT, BinaryOperator::Op::RShift},
};

static const std::map<TokenKind, UnaryOperator::Op> unary_operators = {
    {TokenKind::MINUS, UnaryOperator::Op::Negate},
    {TokenKind::BANG, UnaryOperator::Op::LNot},
    {TokenKind::TIMES, UnaryOperator::Op::Deref},
};

std::unique_ptr<Expression> ExpressionParser::expr() {
    return equality(); 
}

std::unique_ptr<Expression> ExpressionParser::equality() {
    std::unique_ptr<Expression> result = logical();
    while (curtok.kind == TokenKind::EQ
            || curtok.kind == TokenKind::NEQ) {
        auto kind = curtok.kind;
        GetNext();
        auto next = logical();
        result = std::make_unique<BinaryOperator>(std::move(result),
                                                  operators.at(kind),
                                                  std::move(next));
    }
    return result;
}

std::unique_ptr<Expression> ExpressionParser::logical() {
    std::unique_ptr<Expression> result = comparison();
    while (curtok.kind == TokenKind::LAND
            || curtok.kind == TokenKind::LOR) {
        auto kind = curtok.kind;
        GetNext();
        auto next = comparison();
        result = std::make_unique<BinaryOperator>(std::move(result),
                                                  operators.at(kind),
                                                  std::move(next));
    }
    return result;
}

std::unique_ptr<Expression> ExpressionParser::comparison() {
    std::unique_ptr<Expression> result = shifts();
    while (curtok.kind == TokenKind::LESS
            || curtok.kind == TokenKind::GREATER) {
        auto kind = curtok.kind;
        GetNext();
        auto next = shifts();
        result = std::make_unique<BinaryOperator>(std::move(result),
                                                  operators.at(kind),
                                                  std::move(next));
    }
    return result;
}

std::unique_ptr<Expression> ExpressionParser::shifts() {
    std::unique_ptr<Expression> result = bit_ops();
    while (curtok.kind == TokenKind::LSHIFT
            || curtok.kind == TokenKind::RSHIFT) {
        auto kind = curtok.kind;
        GetNext();
        auto next = bit_ops();
        result = std::make_unique<BinaryOperator>(std::move(result),
                                                  operators.at(kind),
                                                  std::move(next));
    }
    return result;
}

std::unique_ptr<Expression> ExpressionParser::bit_ops() {
    std::unique_ptr<Expression> result = term();
    while (curtok.kind == TokenKind::AND
            || curtok.kind == TokenKind::XOR
            || curtok.kind == TokenKind::OR) {
        auto kind = curtok.kind;
        GetNext();
        auto next = term();
        result = std::make_unique<BinaryOperator>(std::move(result),
                                                  operators.at(kind),
                                                  std::move(next));
    }
    return result;
}

std::unique_ptr<Expression> ExpressionParser::term() {
    std::unique_ptr<Expression> result = factor();
    while (curtok.kind == TokenKind::PLUS
            || curtok.kind == TokenKind::MINUS) {
        auto kind = curtok.kind;
        GetNext();
        auto next = factor();
        result = std::make_unique<BinaryOperator>(std::move(result),
                                                  operators.at(kind),
                                                  std::move(next));
    }
    return result;
}

std::unique_ptr<Expression> ExpressionParser::factor() {
    std::unique_ptr<Expression> result = unary();
    while (curtok.kind == TokenKind::TIMES
            || curtok.kind == TokenKind::MOD
            || curtok.kind == TokenKind::SLASH) {
        auto kind = curtok.kind;
        GetNext();
        auto next = unary();
        result = std::make_unique<BinaryOperator>(std::move(result),
                                                  operators.at(kind),
                                                  std::move(next));
    }
    return result;
}

std::unique_ptr<Expression> ExpressionParser::unary() {
    if (unary_operators.contains(curtok.kind)) {
        auto op = unary_operators.at(curtok.kind);
        GetNext();
        auto e = unary();
        return std::make_unique<UnaryOperator>(std::move(e), op);
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
    } else if (curtok.kind == TokenKind::DOLLAR) {
        if (GetNext() != TokenKind::NUM) {
            throw CreateError("Expected an index for $");
        }
        auto num = lex.getNumber();
        GetNext();
        return std::make_unique<EvaluatedExpr>(num);
    } else {
        throw CreateError("Expected either identifier, int or float");
    }
}

