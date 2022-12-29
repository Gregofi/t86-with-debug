#pragma once

#include "common/colors.h"
#include "tinyC/backend/global_object.h"
#include "tinyC/backend/function.h"

#include <map>
#include <string>
#include <memory>
#include <algorithm>
#include <iterator>
#include <ranges>

namespace tinyc {
    class Context {
    public:
        /**
         * Map would be faster for lookup, hovewer, we need to preserve the order
         * in which elements we're inserted (because if function is called it needs
         * to be already known).
         */
        std::vector<std::pair<std::string, GlobalObject*>> globals;

        auto begin() const { return globals.begin(); }
        auto end() const { return globals.end(); }
        auto erase(auto it) { return globals.erase(it); }

        /**
         * @brief Registers the function
         *
         * @param name - Name of the function
         * @param func - Function object
         * @return true - If function wasn't already present.
         * @return false - If function was already present.
         */
        bool addFunction(const std::string& name, Function* func) {
            if (std::any_of(globals.begin(), globals.end(), [name](const auto& pair){ return pair.first == name; })) {
                return false;
            }
            globals.emplace_back(std::move(name), func);
            return true;
        }

        /**
         * Finds function by given name and returns it. If the function
         * does not exist or object with 'name' is not a function return
         * nullptr.
         */
        Function* getFunction(const std::string& name) {
            auto it = std::find_if(globals.begin(), globals.end(), [name](const auto& pair) -> bool {
                return pair.first == name;
            });
            if (it == globals.end()) {
                return nullptr;
            }
            Function* fun = dynamic_cast<Function*>(it->second);
            return fun;
        }

        void dump(colors::ColorPrinter& p) const {
            for (const auto& f: *this) {
                const auto& func = f.second;
                (*func).print(p);
            }
        }
    };
}
