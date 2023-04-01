#include "t86/os.h"
#include "bench_lib.h"
#include <chrono>
#include <iostream>
#include <fstream>

using namespace tiny::t86;

class T86Runner: public bench::Fixture {
public:
    void Run(const std::string& path) {
        OS os;
        std::ifstream fs{TEST_CASE_DIRECTORY + path};
        Parser parser(fs);
        Program p = parser.Parse();
        os.Run(std::move(p));
    }
};

BENCH(T86Runner, T86Quicksort) {
    Run("/benches/quicksort.t86");
}

BENCH(T86Runner, T86Prime) {
    Run("/benches/prime.t86");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fmt::print("usage: {} bench\n", argv[0]);
        return 1;
    }
    RUN_BENCHMARK(T86Quicksort);
    RUN_BENCHMARK(T86Prime);
    fmt::print("Unknown benchmark\n");
}
