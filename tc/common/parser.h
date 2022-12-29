#pragma once

#include "helpers.h"

#include "lexer.h"
#include "ast.h"

namespace tiny {


    class ParserBase {
    public:

    protected:

        class Position {
        private:
            friend class ParserBase;

            Position(size_t i):
                i_{i} {
            }
            size_t i_;
        };

        ParserBase(std::vector<Token> && tokens):
            tokens_{std::move(tokens)},
            i_{0} {
        }

        Position position() const {
            return Position{i_};
        }

        void revertTo(Position const & p) {
            i_ = p.i_;
        }

        bool eof() const {
            return i_ == tokens_.size() - 1;
        }

        Token const & top() const {
            return tokens_[i_];
        }

        Token const & pop() {
            Token const & result = top();
            if (i_ != tokens_.size() - 1)
                ++i_;
            return result;
        }

        Token const & pop(Token::Kind kind) {
            if (top().kind() != kind)
                throw ParserError(STR("Expected " << kind << ", but " << top() << " found"), top().location(), eof());
            return pop();
        }

        Token const & pop(Symbol symbol) {
            if ((top().kind() != Token::Kind::Identifier && top().kind() != Token::Kind::Operator) || top().valueSymbol() != symbol)
                throw ParserError(STR("Expected " << symbol << ", but " << top() << " found"), top().location(), eof());
            return pop();
        }

        bool condPop(Token::Kind kind) {
            if (top().kind() != kind)
                return false;
            pop();
            return true;
        }

        bool condPop(Symbol symbol) {
            if ((top().kind() != Token::Kind::Identifier && top().kind() != Token::Kind::Operator) || top().valueSymbol() != symbol)
                return false;
            pop();
            return true;
        }

        Token const & peek(int offset) {
            int index = static_cast<int>(i_) + offset;
            if (index < 0 || (size_t)index >= tokens_.size())
                index = static_cast<int>(tokens_.size()) - 1;
            return tokens_[index];
        }

    private:

        std::vector<Token> tokens_;
        size_t i_;
    }; // tiny::ParserBase

} // namespace tiny