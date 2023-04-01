/** A pocket benchmark library, inspired by googletest.
 * The google benchmark library proved to heavyweight
 * for our very simple usecase, so we rolled our own.
 */
#pragma once
#include <chrono>
#include <iostream>

namespace bench {
class Fixture {
public:
    virtual void Setup() {

    }

    virtual void Teardown() {
        
    }
};

#define BENCH(FIXTURE, NAME) \
class NAME: public FIXTURE { \
public: \
    void operator()(); \
}; \
void NAME::operator()() \

#define RUN_BENCHMARK(NAME) \
if (!strcmp(argv[1], #NAME)) { \
do { \
    fmt::print("Running bench {}\n", #NAME); \
    NAME bench{}; \
    bench.Setup(); \
    auto t1 = std::chrono::high_resolution_clock::now(); \
    bench(); \
    auto t2 = std::chrono::high_resolution_clock::now(); \
    bench.Teardown(); \
    std::chrono::duration<double> s_double = t2 - t1; \
    fmt::print("Bench: {}, duration: {}s\n", #NAME, s_double.count()); \
    return 0; \
} while (false); }

inline std::string GetPath(std::string_view view) {
    auto pos = view.rfind("/");
    if (pos == std::string_view::npos) {
        return "";
    }
    return std::string{view.substr(0, pos)};
}

#define TEST_CASE_DIRECTORY bench::GetPath(__FILE__)
};
