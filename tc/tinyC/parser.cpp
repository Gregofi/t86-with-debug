#include "parser.h"
#include "frontend.h"

namespace tinyc {

    bool Parser::isTypeName(Symbol name) const {
        return possibleTypes_.find(name) != possibleTypes_.end();
    }

    /* PROGRAM := { FUN_DECL | VAR_DECLS ';' | STRUCT_DECL | FUNPTR_DECL }

        TODO the simple try & fail & try something else produces ugly error messages.
        */
    std::unique_ptr<AST> Parser::PROGRAM() {
        std::unique_ptr<ASTBlock> result{new ASTBlock{top()}};
        while (! eof()) {
            if (top() == Symbol::KwStruct) {
                result->body.push_back(STRUCT_DECL());
            } else if (top() == Symbol::KwTypedef) {
                result->body.push_back(FUNPTR_DECL());
            } else {
                // it can be either function or variable declaration now, we just do the dirty 
                // trick by first parsing the type and identifier to determine whether we're 
                // dealing with a function or variable declaration, then revert the parser and 
                // parser the proper nonterminal this time.
                Position x = position();
                TYPE(true);
                IDENT();
                if (top() == Symbol::ParOpen) {
                    revertTo(x);
                    result->body.push_back(FUN_DECL());
                } else {
                    revertTo(x);
                    result->body.push_back(VAR_DECLS());
                    pop(Symbol::Semicolon);
                }
            }
        }
        return result;
    }

    /* FUN_DECL := TYPE_FUN_RET identifier '(' [ FUN_ARG { ',' FUN_ARG } ] ')' [ BLOCK_STMT ]
        FUN_ARG := TYPE identifier
        */
    std::unique_ptr<AST> Parser::FUN_DECL() {
        std::unique_ptr<ASTType> type{TYPE_FUN_RET()};
        if (!isIdentifier(top()))
            throw ParserError(STR("Expected identifier, but " << top() << " found"), top().location(), eof());
        std::unique_ptr<ASTFunDecl> result{new ASTFunDecl{pop(), std::move(type)}};
        pop(Symbol::ParOpen);
        if (top() != Symbol::ParClose) {
            do {
                std::unique_ptr<ASTType> argType{TYPE()};
                std::unique_ptr<ASTIdentifier> argName{IDENT()};
                // check that the name is unique
                for (auto & i : result->args)
                    if (i.second->name == argName->name)
                        throw ParserError(STR("Function argument " << argName->name << " altready defined"), argName->location(), false);
                result->args.push_back(std::make_pair(std::move(argType), std::move(argName)));

            } while (condPop(Symbol::Comma));
        }
        pop(Symbol::ParClose);
        // if there is body, parse it, otherwise leave empty as it is just a declaration
        if (top() == Symbol::CurlyOpen)
            result->body = BLOCK_STMT();
        return result;
    }

    // Statements -----------------------------------------------------------------------------------------------------

    /* STATEMENT := BLOCK_STMT | IF_STMT | SWITCH_STMT | WHILE_STMT | DO_WHILE_STMT | FOR_STMT | BREAK_STMT | CONTINUE_STMT | RETURN_STMT | EXPR_STMT
        */
    std::unique_ptr<AST> Parser::STATEMENT() {
        if (top() == Symbol::CurlyOpen)
            return BLOCK_STMT();
        else if (top() == Symbol::KwIf)
            return IF_STMT();
        else if (top() == Symbol::KwSwitch)
            return SWITCH_STMT();
        else if (top() == Symbol::KwWhile)
            return WHILE_STMT();
        else if (top() == Symbol::KwDo)
            return DO_WHILE_STMT();
        else if (top() == Symbol::KwFor)
            return FOR_STMT();
        else if (top() == Symbol::KwBreak)
            return BREAK_STMT();
        else if (top() == Symbol::KwContinue)
            return CONTINUE_STMT();
        else if (top() == Symbol::KwReturn)
            return RETURN_STMT();
        else if (top() == symbol::KwScan) {
            Token op =  pop();
            pop(Symbol::ParOpen);
            pop(Symbol::ParClose);
            return std::unique_ptr<AST>{new ASTRead{op}};
        } else if (top() == symbol::KwPrint) {
            Token op =  pop();
            pop(Symbol::ParOpen);
            std::unique_ptr<AST> expr(EXPR());
            pop(Symbol::ParClose);
            return std::unique_ptr<AST>{new ASTWrite{op, std::move(expr)}};
        }
        else
            // TODO this would produce not especially nice error as we are happy with statements too
            return EXPR_STMT();
    }

    /* BLOCK_STMT := '{' { STATEMENT } '}'
        */
    std::unique_ptr<AST> Parser::BLOCK_STMT() {
        std::unique_ptr<ASTBlock> result{new ASTBlock{pop(Symbol::CurlyOpen)}};
        while (!condPop(Symbol::CurlyClose))
            result->body.push_back(STATEMENT());
        return result;
    }

    /* IF_STMT := if '(' EXPR ')' STATEMENT [ else STATEMENT ]
        */
    std::unique_ptr<ASTIf> Parser::IF_STMT() {
        std::unique_ptr<ASTIf> result{new ASTIf{pop(Symbol::KwIf)}};
        pop(Symbol::ParOpen);
        result->cond = EXPR();
        pop(Symbol::ParClose);
        result->trueCase = STATEMENT();
        if (condPop(Symbol::KwElse))
            result->falseCase = STATEMENT();
        return result;
    }

    /* SWITCH_STMT := switch '(' EXPR ')' '{' { CASE_STMT } [ default ':' CASE_BODY ] { CASE_STMT } '}'
        CASE_STMT := case integer_literal ':' CASE_BODY
        */
    std::unique_ptr<AST> Parser::SWITCH_STMT() {
        std::unique_ptr<ASTSwitch> result{new ASTSwitch{pop(Symbol::KwSwitch)}};
        pop(Symbol::ParOpen);
        result->cond = EXPR();
        pop(Symbol::ParClose);
        pop(Symbol::CurlyOpen);
        while (!condPop(Symbol::CurlyClose)) {
            if (top() == Symbol::KwDefault) {
                if (result->defaultCase != nullptr)
                    throw ParserError("Default case already provided", top().location(), false);
                pop();
                pop(Symbol::Colon);
                auto tmp = CASE_BODY();
                result->defaultCase = tmp.get();
                result->cases.emplace_back(0, std::move(tmp));
            } else if (condPop(Symbol::KwCase)) {
                Token const & t = top();
                int value = pop(Token::Kind::Integer).valueInt();
                auto it = result->cases.begin();
                while(it != result->cases.end() && it->first != value ){
                    it++;
                }
//                if (result->cases.find(value) != result->cases.end())
                if(it != result->cases.end())
                    throw ParserError(STR("Case " << value << " already provided"), t.location(), false);
                pop(Symbol::Colon);
                result->cases.emplace_back(value, CASE_BODY());
            } else {
                throw ParserError(STR("Expected case or default keyword but " << top() << " found"), top().location(), eof());
            }
        }
        return result;
    }

    /* CASE_BODY := { STATEMENT }

        Can be empty if followed by case, default, ot `}`.
        */
    std::unique_ptr<AST> Parser::CASE_BODY() {
        std::unique_ptr<ASTBlock> result{ new ASTBlock{top()}};
        while (top() != Symbol::KwCase && top() != Symbol::KwDefault && top() != Symbol::CurlyClose) {
            result->body.push_back(STATEMENT());
        }
        return result;
    }

    /* WHILE_STMT := while '(' EXPR ')' STATEMENT
        */
    std::unique_ptr<AST> Parser::WHILE_STMT() {
        std::unique_ptr<ASTWhile> result{new ASTWhile{pop(Symbol::KwWhile)}};
        pop(Symbol::ParOpen);
        result->cond = EXPR();
        pop(Symbol::ParClose);
        result->body = STATEMENT();
        return result;
    }

    /* DO_WHILE_STMT := do STATEMENT while '(' EXPR ')' ';'
        */
    std::unique_ptr<AST> Parser::DO_WHILE_STMT() {
        std::unique_ptr<ASTDoWhile> result{new ASTDoWhile{pop(Symbol::KwDo)}};
        result->body = STATEMENT();
        pop(Symbol::KwWhile);
        pop(Symbol::ParOpen);
        result->cond = EXPR();
        pop(Symbol::ParClose);
        pop(Symbol::Semicolon);
        return result;
    }

    /* FOR_STMT := for '(' [ EXPR_OR_VAR_DECL ] ';' [ EXPR ] ';' [ EXPR ] ')' STATEMENT
        */
    std::unique_ptr<AST> Parser::FOR_STMT() {
        std::unique_ptr<ASTFor> result{new ASTFor{pop(Symbol::KwFor)}};
        pop(Symbol::ParOpen);
        if (top() != Symbol::Semicolon)
            result->init = EXPR_OR_VAR_DECL();
        pop(Symbol::Semicolon);
        if (top() != Symbol::Semicolon)
            result->cond = EXPR();
        pop(Symbol::Semicolon);
        if (top() != Symbol::ParClose)
            result->increment = EXPR();
        pop(Symbol::ParClose);
        result->body = STATEMENT();
        return result;
    }

    /* BREAK_STMT := break ';'

        The parser allows break statement even when there is no loop or switch around it. This has to be fixed in the translator.
        */
    std::unique_ptr<AST> Parser::BREAK_STMT() {
        std::unique_ptr<AST> result{new ASTBreak{pop(Symbol::KwBreak)}};
        pop(Symbol::Semicolon);
        return result;
    }

    /* CONTINUE_STMT := continue ';'

        The parser allows continue statement even when there is no loop around it. This has to be fixed in the translator.
        */
    std::unique_ptr<AST> Parser::CONTINUE_STMT() {
        std::unique_ptr<AST> result{new ASTContinue{pop(Symbol::KwContinue)}};
        pop(Symbol::Semicolon);
        return result;
    }

    /* RETURN_STMT := return [ EXPR ] ';'
        */
    std::unique_ptr<ASTReturn> Parser::RETURN_STMT() {
        std::unique_ptr<ASTReturn> result{new ASTReturn{pop(Symbol::KwReturn)}};
        if (top() != Symbol::Semicolon)
            result->value = EXPR();
        pop(Symbol::Semicolon);
        return result;
    }

    /* EXPR_STMT := EXPR_OR_VAR_DECL ';'
'         */
    std::unique_ptr<AST> Parser::EXPR_STMT() {
        std::unique_ptr<AST> result{EXPR_OR_VAR_DECL()};
        pop(Symbol::Semicolon);
        return result;
    }

    // Types ----------------------------------------------------------------------------------------------------------

    /* TYPE := (int | double | char | identifier) { * }
            |= void * { * }

        The identifier must be a typename.
        */
    std::unique_ptr<ASTType> Parser::TYPE(bool canBeVoid) {
        std::unique_ptr<ASTType> result;
        if (top() == Symbol::KwVoid) {
            result.reset(new ASTNamedType{pop()});
            // if it can't be void, it must be void*
            if (!canBeVoid)
                result.reset(new ASTPointerType{pop(Symbol::Mul), std::move(result)});
        } else {
            if (top() == Symbol::KwInt || top() == Symbol::KwChar || top() == Symbol::KwDouble)
                result.reset(new ASTNamedType{pop()});
            else if (isIdentifier(top()) && isTypeName(top().valueSymbol()))
                result.reset(new ASTNamedType{pop()});
            else
                throw ParserError(STR("Expected type, but " << top() << " found"), top().location(), eof());
        }
        // deal with pointers to pointers
        while (top() == Symbol::Mul)
            result.reset(new ASTPointerType{pop(Symbol::Mul), std::move(result)});
        return result;
    }

    /* TYPE_FUN_RET := void | TYPE
        */
    std::unique_ptr<ASTType> Parser::TYPE_FUN_RET() {
        return TYPE(true);
    }

    // Type Declarations ----------------------------------------------------------------------------------------------

    /* STRUCT_TYPE_DECL := struct identifier [ '{' { TYPE identifier ';' } '}' ] ';'
        */
    std::unique_ptr<ASTStructDecl> Parser::STRUCT_DECL() {
        Token const & start = pop(Symbol::KwStruct);
        std::unique_ptr<ASTStructDecl> decl{new ASTStructDecl{start, pop(Token::Kind::Identifier).valueSymbol()}};
        addTypeName(decl->name);
        if (condPop(Symbol::CurlyOpen)) {
            while (! condPop(Symbol::CurlyClose)) {
                std::unique_ptr<ASTType> type{TYPE()};
                decl->fields.push_back(std::make_pair(IDENT(), std::move(type)));
                if (condPop(Symbol::SquareOpen)) {
                    std::unique_ptr<AST> index{E9()};
                    pop(Symbol::SquareClose);
                    // now we have to update the type
                    decl->fields.back().second.reset(new ASTArrayType{start, std::move(decl->fields.back().second), std::move(index) });
                }
                pop(Symbol::Semicolon);
            }
            decl->isDefinition = true;
        }
        pop(Symbol::Semicolon);
        return decl;
    }

    /* FUNPTR_TYPE_DECL := typedef TYPE_FUN_RET '(' '*' identifier ')' '(' [ TYPE { ',' TYPE } ] ')' ';'
        */

    std::unique_ptr<ASTFunPtrDecl> Parser::FUNPTR_DECL() {
        Token const & start = pop(Symbol::KwTypedef);
        std::unique_ptr<ASTType> returnType{TYPE_FUN_RET()};
        pop(Symbol::ParOpen);
        pop(Symbol::Mul);
        std::unique_ptr<ASTIdentifier> name{IDENT()};
        addTypeName(name->name);
        pop(Symbol::ParClose);
        pop(Symbol::ParOpen);
        std::unique_ptr<ASTFunPtrDecl> result{new ASTFunPtrDecl{start, std::move(name), std::move(returnType)}};
        if (top() != Symbol::ParClose) {
            result->args.push_back(TYPE());
            while (condPop(Symbol::Comma))
                result->args.push_back(TYPE());
        }
        pop(Symbol::ParClose);
        pop(Symbol::Semicolon);
        return result;
    }

    // Expressions ----------------------------------------------------------------------------------------------------

    /* EXPR_OR_VAR_DECL := ( EXPR | VAR_DECL)  { ',' ( EXPR | VAR_DECL ) }

        We can either be smart and play with FIRST and FOLLOW sets, or we can be lazy and just do the simple thing - try TYPE first and if it fails, try EXPR.

        I am lazy.
        */
    std::unique_ptr<AST> Parser::EXPR_OR_VAR_DECL() {
        Position x = position();
        try {
            TYPE();
        } catch (...) {
            revertTo(x);
            return EXPRS();
        }
        revertTo(x);
        return VAR_DECLS();
    }

    /* VAR_DECL := TYPE identifier [ '[' E9 ']' ] [ '=' EXPR ]
        */
    std::unique_ptr<ASTVarDecl> Parser::VAR_DECL() {
        Token const & start = top();
        std::unique_ptr<ASTVarDecl> decl{new ASTVarDecl{start, TYPE()}};
        decl->name = IDENT();
        if (condPop(Symbol::SquareOpen)) {
            std::unique_ptr<AST> index{E9()};
            pop(Symbol::SquareClose);
            // now we have to update the type
            decl->varType.reset(new ASTArrayType{start, std::move(decl->varType), std::move(index) });
        }
        if (condPop(Symbol::Assign))
            decl->value = EXPR();
        return decl;
    }

    /* VAR_DECLS := VAR_DECL { ',' VAR_DECL }
        */
    std::unique_ptr<AST> Parser::VAR_DECLS() {
        std::unique_ptr<ASTSequence> result{new ASTSequence{top()}};
        result->body.push_back(VAR_DECL());
        while (condPop(Symbol::Comma))
            result->body.push_back(VAR_DECL());
        return result;
    }

    /* EXPR := E9 [ '=' EXPR ]

        Note that assignment is right associative.
        */
    std::unique_ptr<AST> Parser::EXPR() {
        std::unique_ptr<AST> result{E9()};
        if (top() == Symbol::Assign) {
            Token const & op = pop();
            result.reset(new ASTAssignment{op, std::move(result), EXPR()});
        }
        return result;
    }

    /* EXPRS := EXPR { ',' EXPR }
        */
    std::unique_ptr<AST> Parser::EXPRS() {
        std::unique_ptr<ASTSequence> result{new ASTSequence{top()}};
        result->body.push_back(EXPR());
        while (condPop(Symbol::Comma))
            result->body.push_back(EXPR());
        return result;
    }


    /* E9 := E8 { '||' E8 }
        */
    std::unique_ptr<AST> Parser::E9() {
        std::unique_ptr<AST> result{E8()};
        while (top() == Symbol::Or) {
            Token const & op = pop();
            result.reset(new ASTBinaryOp{op, std::move(result), E8()});
        }
        return result;
    }

    /* E8 := E7 { '&&' E7 }
        */
    std::unique_ptr<AST> Parser::E8() {
        std::unique_ptr<AST> result{E7()};
        while (top() == Symbol::And) {
            Token const & op = pop();
            result.reset(new ASTBinaryOp{op, std::move(result), E7()});
        }
        return result;
    }

    /* E7 := E6 { '|' E6 }
        */
    std::unique_ptr<AST> Parser::E7() {
        std::unique_ptr<AST> result{E6()};
        while (top() == Symbol::BitOr) {
            Token const & op = pop();
            result.reset(new ASTBinaryOp{op, std::move(result), E6()});
        }
        return result;
    }

    /* E6 := E5 { '&' E5 }
        */
    std::unique_ptr<AST> Parser::E6() {
        std::unique_ptr<AST> result{E5()};
        while (top() == Symbol::BitAnd) {
            Token const & op = pop();
            result.reset(new ASTBinaryOp{op, std::move(result), E5()});
        }
        return result;
    }

    /* E5 := E4 { ('==' | '!=') E4 }
        */
    std::unique_ptr<AST> Parser::E5() {
        std::unique_ptr<AST> result{E4()};
        while (top() == Symbol::Eq || top() == Symbol::NEq) {
            Token const & op = pop();
            result.reset(new ASTBinaryOp{op, std::move(result), E4()});
        }
        return result;
    }

    /* E4 := E3 { ('<' | '<=' | '>' | '>=') E3 }
        */
    std::unique_ptr<AST> Parser::E4() {
        std::unique_ptr<AST> result{E3()};
        while (top() == Symbol::Lt || top() == Symbol::Lte || top() == Symbol::Gt || top() == Symbol::Gte) {
            Token const & op = pop();
            result.reset(new ASTBinaryOp{op, std::move(result), E3()});
        }
        return result;
    }

    /* E3 := E2 { ('<<' | '>>') E2 }
        */
    std::unique_ptr<AST> Parser::E3() {
        std::unique_ptr<AST> result{E2()};
        while (top() == Symbol::ShiftLeft || top() == Symbol::ShiftRight) {
            Token const & op = pop();
            result.reset(new ASTBinaryOp{op, std::move(result), E2()});
        }
        return result;
    }

    /* E2 := E1 { ('+' | '-') E1 }
        */
    std::unique_ptr<AST> Parser::E2() {
        std::unique_ptr<AST> result{E1()};
        while (top() == Symbol::Add || top() == Symbol::Sub) {
            Token const & op = pop();
            result.reset(new ASTBinaryOp{op, std::move(result), E1()});
        }
        return result;
    }

    /* E1 := E_UNARY_PRE { ('*' | '/' | '%' ) E_UNARY_PRE }
        */
    std::unique_ptr<AST> Parser::E1() {
        std::unique_ptr<AST> result{E_UNARY_PRE()};
        while (top() == Symbol::Mul || top() == Symbol::Div || top() == Symbol::Mod) {
            Token const & op = pop();
            result.reset(new ASTBinaryOp{op, std::move(result), E_UNARY_PRE()});
        }
        return result;
    }

    /* E_UNARY_PRE := { '+' | '-' | '!' | '~' | '++' | '--' | '*' | '&' } E_CALL_INDEX_MEMBER_POST
        */
    std::unique_ptr<AST> Parser::E_UNARY_PRE() {
        if (top() == Symbol::Add || top() == Symbol::Sub
                || top() == Symbol::Not
                || top() == Symbol::Neg
                || top() == Symbol::Inc
                || top() == Symbol::Dec) {
            Token const & op = pop();
            return std::unique_ptr<AST>{new ASTUnaryOp{op, E_UNARY_PRE()}};
        } else if (top() == Symbol::Mul) {
            Token const & op = pop();
            return std::unique_ptr<AST>{new ASTDeref{op, E_UNARY_PRE()}};
        } else if (top() == Symbol::BitAnd) {
            Token const & op = pop();
            return std::unique_ptr<AST>{new ASTAddress{op, E_UNARY_PRE()}};
        } else {
            return E_CALL_INDEX_MEMBER_POST();
        }
    }

    /* E_CALL_INDEX_MEMBER_POST := F { E_CALL | E_INDEX | E_MEMBER | E_POST }
        E_CALL := '(' [ EXPR { ',' EXPR } ] ')'
        E_INDEX := '[' EXPR ']'
        E_MEMBER := ('.' | '->') identifier
        E_POST := '++' | '--'
        */
    std::unique_ptr<AST> Parser::E_CALL_INDEX_MEMBER_POST() {
        std::unique_ptr<AST> result{F()};
        while (true) {
            if (top() == Symbol::ParOpen) {
                std::unique_ptr<ASTCall> call{new ASTCall{pop(), std::move(result)}};
                if (top() != Symbol::ParClose) {
                    call->args.push_back(EXPR());
                    while (condPop(Symbol::Comma))
                        call->args.push_back(EXPR());
                }
                pop(Symbol::ParClose);
                result.reset(call.release());
            } else if (top() == Symbol::SquareOpen) {
                Token const & op = pop();
                result.reset(new ASTIndex{op, std::move(result), EXPR()});
                pop(Symbol::SquareClose);
            } else if (top() == Symbol::Dot) {
                Token const & op = pop();
                result.reset(new ASTMember{op, std::move(result), pop(Token::Kind::Identifier).valueSymbol()});
            } else if (top() == Symbol::ArrowR) {
                Token const & op = pop();
                result.reset(new ASTMemberPtr{op, std::move(result), pop(Token::Kind::Identifier).valueSymbol()});
            } else if (top() == Symbol::Inc || top() == Symbol::Dec) {
                Token const & op = pop();
                result.reset(new ASTUnaryPostOp{op, std::move(result)});
            } else {
                break;
            }
        }
        return result;
    }

    /* F := integer | double | char | string | identifier | '(' EXPR ')' | E_CAST
        E_CAST := cast '<' TYPE '>' '(' EXPR ')'
        */
    std::unique_ptr<AST> Parser::F() {
        if (top() == Token::Kind::Integer) {
            return std::unique_ptr<AST>{new ASTInteger{pop()}};
        } else if (top() == Token::Kind::Double) {
            return std::unique_ptr<AST>{new ASTDouble{pop()}};
        } else if (top() == Token::Kind::StringSingleQuoted) {
            return std::unique_ptr<AST>{new ASTChar{pop()}};
        } else if (top() == Token::Kind::StringDoubleQuoted) {
            return std::unique_ptr<AST>{new ASTString{pop()}};
        } else if (top() == Symbol::KwCast) {
            Token op = pop();
            pop(Symbol::Lt);
            std::unique_ptr<ASTType> type{TYPE()};
            pop(Symbol::Gt);
            pop(Symbol::ParOpen);
            std::unique_ptr<AST> expr(EXPR());
            pop(Symbol::ParClose);
            return std::unique_ptr<AST>{new ASTCast{op, std::move(expr), std::move(type)}};
        }
        else if (top() == Token::Kind::Identifier) {
            return IDENT();
        } else {
            throw ParserError(STR("Expected literal, (expr) or cast, but " << top() << " found"), top().location(), eof());
        }
    }

    std::unique_ptr<ASTIdentifier> Parser::IDENT() {
        if (!isIdentifier(top()) || isTypeName(top().valueSymbol()))
            throw ParserError(STR("Expected identifier, but " << top() << " found"), top().location(), eof());
        return std::unique_ptr<ASTIdentifier>{new ASTIdentifier{pop()}};
    }

    bool symbol::isKeyword(const Symbol &s) {
        return s == KwPrint
               || s == KwScan
                ;
    }
}

