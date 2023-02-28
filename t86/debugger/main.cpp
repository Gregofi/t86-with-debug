#include "CLI.h"
#include "Native.h"

int main(int argc, char* argv[]) {
    // Initialize connection to debuggee
    auto debuggee_process = Native::Initialize(9110);
    // Set up the native manager
    Native native(std::move(debuggee_process));
    // Run CLI
    CLI cli(std::move(native));
    return cli.Run();
}
