#pragma once

#include <exception>
#include <string>
#include <stdexcept>

namespace tinyc {
    class CodegenError: public std::runtime_error {
    public:
        explicit CodegenError(std::string message) : runtime_error(std::move(message)) {

        }
    };
}
