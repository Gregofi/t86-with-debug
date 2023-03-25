#include "parsing.h"

TokenKind Lexer::ParseNumber() {
    bool is_float = false;
    std::string num{lookahead};
    while (true) {
        lookahead = GetChar();
        if (lookahead == '.') {
            is_float = true;

        } else if (!isdigit(lookahead)) {
            break;
        }
        num += lookahead;
    }
    if (is_float) {
        float_number = std::stod(num);
        return TokenKind::FLOAT;
    } else {
        number = std::stoi(num);
        return TokenKind::NUM;
    }
}

void Lexer::ParseString() {
    str.clear();
    while (GetChar() != '"') {
        if (lookahead == EOF) {
            throw ParserError("Unterminated string!");
        } else if (lookahead == '\\') {
            GetChar();
            if (lookahead == 'n') {
                str += '\n';
            } else if (lookahead == 't') {
                str += '\t';
            } else if (lookahead == '\\') {
                str += '\\';
            } else if (lookahead == '\"') {
                str += '\"';
            } else {
                throw ParserError(fmt::format("Unknown escape sequence: '\\{}'", lookahead));
            }
        } else {
            str += lookahead;
        }
    }
    GetChar();
}

Token Lexer::getNext() {
    if (lookahead == '#') {
        while (lookahead != EOF && lookahead != '\n') {
            GetChar();
        }
        if (lookahead == '\n') {
            GetChar();
        }
        return getNext();
    } else if (isspace(lookahead)) {
        GetChar();
        return getNext();
    }

    RecordTokLoc();
    if (lookahead == EOF) {
        return MakeToken(TokenKind::END);
    } else if (lookahead == ';') {
        GetChar();
        return MakeToken(TokenKind::SEMICOLON);
    } else if (lookahead == ',') {
        GetChar();
        return MakeToken(TokenKind::COMMA);
    } else if (lookahead == '[') {
        GetChar();
        return MakeToken(TokenKind::LBRACKET);
    } else if (lookahead == ']') {
        GetChar();
        return MakeToken(TokenKind::RBRACKET);
    } else if (lookahead == '{') {
        GetChar();
        return MakeToken(TokenKind::LBRACE);
    } else if (lookahead == '}') {
        GetChar();
        return MakeToken(TokenKind::RBRACE);
    } else if (lookahead == '(') {
        GetChar();
        return MakeToken(TokenKind::LPAREN);
    } else if (lookahead == ')') {
        GetChar();
        return MakeToken(TokenKind::RPAREN);
    } else if (lookahead == '!') {
        GetChar();
        if (lookahead == '=') {
            GetChar();
            return MakeToken(TokenKind::NEQ);
        }
        return MakeToken(TokenKind::BANG);
    } else if (lookahead == '=') {
        GetChar();
        if (lookahead == '=') {
            GetChar();
            MakeToken(TokenKind::EQ);
        }
        return MakeToken(TokenKind::ASSIGN);
    } else if (lookahead == '%') {
        GetChar();
        return MakeToken(TokenKind::MOD);
    } else if (lookahead == '+') {
        GetChar();
       return MakeToken(TokenKind::PLUS);
    } else if (lookahead == '-') {
        GetChar();
        if (lookahead == '>') {
            GetChar();
            return MakeToken(TokenKind::ARROW);
        }
       return MakeToken(TokenKind::MINUS);
    } else if (lookahead == '>') {
        GetChar();
        if (lookahead == '=') {
            GetChar();
            return MakeToken(TokenKind::GEQ);
        } else if (lookahead == '>') {
            GetChar();
            return MakeToken(TokenKind::RSHIFT);
        }
        return MakeToken(TokenKind::GREATER);
    } else if (lookahead == '<') {
        GetChar();
        if (lookahead == '=') {
            GetChar();
            return MakeToken(TokenKind::LEQ);
        } else if (lookahead == '<') {
            GetChar();
            return MakeToken(TokenKind::LSHIFT);
        }
        return MakeToken(TokenKind::LESS);
    } else if (lookahead == ':') {
        GetChar();
        return MakeToken(TokenKind::DOUBLEDOT);
    } else if (lookahead == '/') {
        GetChar();
        return MakeToken(TokenKind::SLASH);
    } else if (lookahead == '*') {
        GetChar();
        return MakeToken(TokenKind::TIMES);
    } else if (lookahead == '`') {
        GetChar();
        return MakeToken(TokenKind::BACKTICK);
    } else if (lookahead == '$') {
        GetChar();
        return MakeToken(TokenKind::DOLLAR);
    } else if (lookahead == '.') {
        GetChar();
        return MakeToken(TokenKind::DOT);
    } else if (lookahead == '"') {
        ParseString();
        return MakeToken(TokenKind::STRING);
    } else if (lookahead == '&') {
        if (GetChar() == '&') {
            GetChar();
            return MakeToken(TokenKind::LAND);
        }
        return MakeToken(TokenKind::AND);
    } else if (lookahead == '|') {
        if (GetChar() == '|') {
            GetChar();
            return MakeToken(TokenKind::LOR);
        }
        return MakeToken(TokenKind::OR);
    } else if (lookahead == '^') {
        GetChar();
        return MakeToken(TokenKind::XOR);
    } else if (isdigit(lookahead)) {
        return MakeToken(ParseNumber());
    } else if (isalpha(lookahead) || lookahead == '_') { // identifier
        ParseIdentifier();
        return MakeToken(TokenKind::ID);
    } else {
        if (ignore) {
            GetChar();
            return getNext();
        } else {
            throw ParserError(fmt::format("{}:{}:No token beginning with '{}'",
                                          row, col, lookahead));
        }
    }
}
