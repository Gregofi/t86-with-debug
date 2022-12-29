#include "frontend.h"
#include "tinyC/typechecker.h"

namespace tinyc {

    Frontend::Frontend() = default;

    void Frontend::typecheck(AST * ast) {
        Typechecker tp;
        tp.visit(ast);
    }

} // namespace tiny

