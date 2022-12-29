#pragma once

#include <map>
#include <string>
#include <vector>
#include <optional>


template<typename T>
class Environment {
    std::vector<std::map<std::string, T>> envs;
public:
    bool is_empty() const { return envs.empty(); }

    void push_env() {
        envs.emplace_back();
    }

    void pop_env() {
        assert(!envs.empty());
        envs.pop_back();
    }

    void add(std::string name, T val) {
        assert(!envs.empty());
        envs.back().emplace(std::move(name), std::move(val));
    }

    bool contains(const std::string& name) const {
        assert(!envs.empty());
        return get(name).has_value();
    }

    std::optional<T> get(const std::string& name) {
        assert(!envs.empty());
        for (auto &env: envs) {
            if (env.contains(name)) {
                return std::make_optional<T>(env.at(name));
            }
        }
        return std::nullopt;
    }
};
