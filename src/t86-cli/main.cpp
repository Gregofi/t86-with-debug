#include <fstream>
#include <iostream>
#include <argparse/argparse.hpp>

#include "TCP.h"
#include "t86-parser/parser.h"
#include "t86/os.h"

using namespace tiny::t86;

static const int DEFAULT_DBG_PORT = 9110;

const char* usage_str = R"(
Usage: t86-cli command
commands:
    run input - Parses input, which must be valid T86 assembly file,
                and runs it on the VM.
)";

int main(int argc, char* argv[]) {
    argparse::ArgumentParser args("t86-cli");

    args.add_argument("file")
        .help("input file containing t86 assembly");

    args.add_argument("--debug")
        .help("open debugging port at 9110")
        .default_value(false)
        .implicit_value(true);

    args.add_argument("--register-cnt")
        .help("number of general purpose registers")
        .default_value((size_t)8)
        .scan<'u', size_t>();

    args.add_argument("--float-register-cnt")
        .help("number of float registers")
        .default_value((size_t)4)
        .scan<'u', size_t>();

    args.add_argument("--memory-size")
        .help("RAM memory size")
        .default_value((size_t)1024)
        .scan<'u', size_t>();

    try {
        args.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << "\n";
        std::cerr << args;
        std::exit(1);
    }

    std::string filename = args.get<std::string>("file");
    std::fstream f(filename);
    if (!f) {
        std::cerr << "Unable to open file `" << filename << "`\n";
        return 3;
    }

    Parser parser(f);
    tiny::t86::Program program;
    try {
        program = parser.Parse();
        // program.dump();
    } catch (ParserError &err) {
        std::cerr << err.what() << std::endl;
        return 2;
    }

    size_t regs = args.get<size_t>("--register-cnt");
    size_t fltregs = args.get<size_t>("--float-register-cnt");
    size_t memsize = args.get<size_t>("--memory-size");

    OS os(regs, fltregs, memsize);

    if (args["debug"] == true) {
        auto m = std::make_unique<TCP::TCPServer>(DEFAULT_DBG_PORT);
        m->Initialize();
        os.SetDebuggerComms(std::move(m));
        log_info("Listening for debugger connections");
    }

    os.Run(std::move(program));
}
