#include <argparse/argparse.hpp>
#include <fstream>
#include <thread>
#include <fmt/core.h>

#include "CLI.h"
#include "debugger/Native.h"
#include "debugger/Source/Parser.h"
#include "t86-parser/parser.h"
#include "t86/os.h"
#include "threads_messenger.h"

dbg::DebuggingInfo ParseDebugInfo(std::ifstream& elf) {
    dbg::Parser p(elf);
    return p.Parse();
}

SourceFile ParseSourceFile(std::ifstream& ifs) {
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    argparse::ArgumentParser args("debugger");
    args.add_argument("file")
        .help("Input file with T86 assembly to be debugged.");
    
    try {
        args.parse_args(argc, argv);
    } catch (const std::runtime_error& e) {
        fmt::print(stderr, "{}\n", e.what());
        std::cerr << args;
        return 1;
    }
    
    Cli cli(args.get<std::string>("file"));
    cli.Run();
}
