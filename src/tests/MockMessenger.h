#pragma once
#include <vector>
#include <queue>
#include <string>
#include <optional>

#include "messenger.h"

class Comms: public Messenger {
public:
    Comms(std::queue<std::string> in, std::vector<std::string>& out): in(std::move(in)), out(out) { }
    void Send(const std::string&s) override {
        out.push_back(s);
    }

    std::optional<std::string> Receive() override {
        if (in.empty()) {
            return std::nullopt;
        }
        auto x = in.front();
        in.pop();
        return x;
    }
    std::queue<std::string> in;
    std::vector<std::string>& out;
};


