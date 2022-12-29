#include "tinyC/backend/function.h"
#include "tinyC/backend/context.h"
#include <string>

namespace tinyc {
    Function* Function::CreateFunction(std::string name, tiny::Type* ret_type,
                                       std::vector<FunctionArgument*> params,
                                       Context& context, std::string id, bool is_main) {
        /* Can't use make_shared because constructor is private. */
        auto* fun = new Function(ret_type, std::move(params), std::move(id), is_main);
        context.addFunction(std::move(name), fun);
        return fun;
    }
}
