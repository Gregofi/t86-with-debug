#pragma once
#include "common/parsing.h"
#include "debugger/Source/Expression.h"

/// Parses the debugging expression.
///
/// The grammar:
/// Lexical syntax:
///  Number = Integer and floating point numbers.
///  ID = Identifier containing letter, numbers and underscores.
///       Can't begin with a number.
///                  
/// Context-free syntax:
///  expr = equality
///  equality = logical {("==" | "!=") logical}
///  logical = comparison {("&&" | "||") comparison}
///  comparison = term {("<" | "<=" | ">=" | ">") term}
///  shifts = bit_ops {("<<" | ">>") bit_ops}
///  bit_ops = term {("|" | "&" | "^") term}
///  term = factor {("+" | "-") factor}
///  factor = unary {("*" | "/" | "%") unary}
///  unary = ["*"] postfix
///  postfix = primary ([ "[" expr "]" ] | "->" ID | "." ID)
///  primary = Number | ID | "(" expr ")"
class ExpressionParser {
public:
    ExpressionParser(std::istream& is): lex(is) { GetNext(); }
    std::unique_ptr<Expression> ParseExpression() {
        return expr();
    }
private:
    template<typename ...Args>
    ParserError CreateError(fmt::format_string<Args...> format, Args&& ...args) const {
        return ParserError(fmt::format("Error:{}:{}:{}", curtok.row,
                    curtok.col, fmt::format(format,
                        std::forward<Args>(args)...)));
    }

    std::unique_ptr<Expression> expr();
    std::unique_ptr<Expression> equality();
    std::unique_ptr<Expression> logical();
    std::unique_ptr<Expression> comparison();
    std::unique_ptr<Expression> bit_ops();
    std::unique_ptr<Expression> shifts();
    std::unique_ptr<Expression> term();
    std::unique_ptr<Expression> factor();
    std::unique_ptr<Expression> unary();
    std::unique_ptr<Expression> postfix();
    std::unique_ptr<Expression> primary();

    TokenKind GetNext() {
        return (curtok = lex.getNext()).kind;
    }
    Lexer lex;
    Token curtok;
};
