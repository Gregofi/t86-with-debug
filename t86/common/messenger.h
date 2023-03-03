#pragma once
#include <optional>
#include <string>
#include <string_view>

class Messenger {
public:
    virtual std::optional<std::string> Receive() = 0;
    virtual void Send(const std::string& s) = 0;
    virtual ~Messenger() = default;
};