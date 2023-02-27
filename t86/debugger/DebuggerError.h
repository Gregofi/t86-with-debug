#pragma once
#include <exception>
#include <string>

class DebuggerError: public std::exception {
    std::string message;
public:
    DebuggerError(std::string message): message(std::move(message)) {}
    const char* what() const noexcept override { return message.c_str(); }
};
