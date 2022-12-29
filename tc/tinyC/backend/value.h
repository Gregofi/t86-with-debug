#pragma once

#include "common/colors.h"
#include "common.h"
#include <algorithm>

namespace tinyc {
    class IRVisitor;

    /**
     * Most basic class in tinyC.
     * Represents operands to other instructions.
     * Can represent labels, computed values and so on.
     * It however semantically represents instructions or
     * specific code segments (functions, labels...).
     *
     */

    class Value {
    protected:
        /**
         * Unique identifier for printing.
         */
        std::string ID;
        size_t num_id;
        static long unsigned i;

    public:
        virtual void updateUsage(Value* new_val, Value* old_val) {
            // No-op by default;
        }

        Value() {
            num_id = i;
            ID = std::to_string(i++);
        }

        /**
         * Initializes value and assignes it given name,
         * this name is only used in printing.
         * @param id
         */
        explicit Value(std::string id) : ID(std::move(id)) {
            num_id = i++;
            if (std::any_of(id.begin(), id.end(), [](char c) {
                return isdigit(c);
            })) {
                throw CodegenError("Value ID can't contain digits");
            }
        }

        virtual ~Value() = default;

        virtual void accept(IRVisitor& v) = 0;

        virtual std::string toStringID() const {
            return "%" + ID;
        }

        size_t getID() const {
            return num_id;
        }

        // FIXME: Should be const
        virtual colors::ColorPrinter& print(colors::ColorPrinter& printer) = 0;
    };
}