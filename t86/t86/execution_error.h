#include <exception>
#include <string>

namespace tiny::t86 {
class T86ExecutionError : public std::exception {
public:
    T86ExecutionError(std::string message)
        : message(std::move(message)) { }
    const char* what() const noexcept override { return message.c_str(); }

private:
    std::string message;
};
} // namespace tiny::t86
