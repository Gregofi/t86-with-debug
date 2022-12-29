#include "ast.h"

namespace tinyc {

    using namespace colors;



    bool ASTBinaryOp::hasAddress() const {
        return false;
    }

    bool ASTUnaryOp::hasAddress() const {
        return false;
    }



    void ASTString::print(colors::ColorPrinter & p) const {
        p << LITERAL(value);
    }

    void ASTPointerType::print(colors::ColorPrinter & p) const {
        p << (*base) << SYMBOL("*");
    }

    void ASTArrayType::print(colors::ColorPrinter & p) const {
        p << (*base) << SYMBOL("[") << *size << SYMBOL("]");
    }

    void ASTNamedType::print(colors::ColorPrinter & p) const  {
        p << TYPE(name.name());
    }

    void ASTSequence::print(colors::ColorPrinter & p) const {
        auto i = body.begin();
        if (i != body.end()) {
            p << **i;
            while (++i != body.end())
                p << SYMBOL(", ") << **i;
        }
    }

    void ASTBlock::print(colors::ColorPrinter & p) const {
        p << SYMBOL("{") << INDENT;
        for (auto & i : body) {
            p << NEWLINE << *i;
        }
        p << DEDENT << NEWLINE << SYMBOL("}") << NEWLINE;
    }

    void ASTStructDecl::print(colors::ColorPrinter & p) const {
        p << KEYWORD("struct") << name;
        if (isDefinition) {
            p << SYMBOL("{") << INDENT;
            for (auto & i : fields) {
                p << NEWLINE << (*i.first) << " " << *(i.second) << SYMBOL(";");
            }
            p << DEDENT << NEWLINE << SYMBOL("}");
        }
    }

    void ASTSwitch::print(colors::ColorPrinter & p) const {
        p << KEYWORD("switch") << SYMBOL("(") << *cond << SYMBOL(") {") << INDENT; 
        for (auto & i : cases) 
            p << NEWLINE << KEYWORD("case") << i.first << SYMBOL(": ") << *i.second;
        if (defaultCase != nullptr) 
            p << NEWLINE << KEYWORD("default") << SYMBOL(":") << *defaultCase;
        p << DEDENT << NEWLINE << SYMBOL("}");
    }



}