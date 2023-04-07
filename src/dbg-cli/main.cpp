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

const char* usage =
R"(usage: dbg-cli [exe-file]

Provide the file if you want to debug local executable.
For help on debugger uses, run the dbg-cli and type
'help'.
)";

int main(int argc, char* argv[]) {
    std::optional<std::string> fname;
    if (argc > 1) {
        fname = argv[1];
    }
    if (fname && fname.value()[0] == '-') {
        fmt::print(stderr, "{}", usage);
        return 1;
    }
    
    Cli cli(fname);
    cli.Run();
}
