#pragma once

#include <ostream>
#include <functional>

#include "lexer.h"
#include "colors.h"

namespace tiny {

    /** Base class for AST nodes.

        Serves as a base class for the

     */
    class ASTBase {
    public:
        virtual ~ASTBase() = default;

        ASTBase(Token const & t):
            l_{t.location()} {
        }

        SourceLocation location() const {
            return l_;
        }

        virtual void print(colors::ColorPrinter & p) const = 0;

    protected:

    private:
        SourceLocation l_;

    };

} // namespace tiny

inline colors::ColorPrinter & operator << (colors::ColorPrinter & p, tiny::ASTBase const & what) {
    what.print(p);
    return p;
}

