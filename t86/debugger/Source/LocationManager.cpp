#include <algorithm>
#include <numeric>
#include <optional>
#include <cstdint>
#include <vector>
#include "LocationManager.h"

std::optional<uint64_t> LocationManager::GetAddress(size_t source_line) const {
    auto it = location_mapping.find(source_line);
    if (it == location_mapping.end()) {
        return {};
    }
    return it->second;
}

std::vector<size_t> LocationManager::GetLines(uint64_t address) const {
    return std::accumulate(location_mapping.begin(), location_mapping.end(),
            std::vector<size_t>(), [address](std::vector<size_t> acc, auto&& mapping) {
        if (mapping.second == address) {
            acc.push_back(mapping.first);
        }
        return std::move(acc);
    });
}
