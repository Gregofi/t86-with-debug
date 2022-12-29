#pragma once

#include <string>

#include "ast.h"
#include "parser.h"
#include "typechecker.h"

namespace tinyc {

    /** The TinyC frontend.

     */
    class Frontend {
    public:

        Frontend();

        /** Opens file specified by the filename and parses the tinyC source within.
         */
        std::unique_ptr<AST> parseFile(std::string const & filename) {
            return Parser::ParseFile(filename, *this);
        }

        std::unique_ptr<AST> parse(std::string const & source) {
            return Parser::Parse(source, *this);
        }

        /** TODO We'll deal with this in Lecture 2.
         */
        static void typecheck(AST * ast);

        void typecheck(std::unique_ptr<AST> const & ast) { typecheck(ast.get()); }

    }; // tinyc::Frontend
}

