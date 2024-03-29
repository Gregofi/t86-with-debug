#pragma once
#include <cstdint>
#include <map>
#include <vector>
#include <optional>

/// Provides a mapping from address to program lines.
class LineMapping {
public:
    LineMapping(std::map<size_t, uint64_t> mapping) : location_mapping(std::move(mapping)) {}

    /// Returns an address that maps to given line
    std::optional<uint64_t> GetAddress(size_t source_line) const;
    /// Returns vector of lines that maps to given address
    std::vector<size_t> GetLines(uint64_t address) const;
private:
    /// Maps locations from source lines onto addresses
    std::map<size_t, uint64_t> location_mapping;
};
