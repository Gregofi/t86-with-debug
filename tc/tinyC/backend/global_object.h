#pragma once

#include "tinyC/backend/value.h"
#include "tinyC/backend/constant.h"
#include <string>

namespace tinyc {

    /**
    * Represents global objects (ie. functions or global variables).
    */
    class GlobalObject: public Value {
    protected:
    public:
        GlobalObject(std::string ID) : Value(std::move(ID)) {}
    };
}
