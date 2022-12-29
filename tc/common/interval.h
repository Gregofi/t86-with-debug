#pragma once

#include <utility>
#include <cstdlib>

class Interval {
    std::pair<size_t, size_t> interval;
public:
    Interval(size_t begin, size_t end) : interval(std::make_pair(begin, end)) {

    }

    size_t getBegin() const { return interval.first; }

    size_t getEnd() const { return interval.second; }

    size_t& getBegin() { return interval.first; }

    size_t& getEnd() { return interval.second; }
};