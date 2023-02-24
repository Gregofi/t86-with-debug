#include <iostream>

#include "common/TCP.h"
#include "common/logger.h"

int main() {
    TCP::TCPClient c(9110);
    c.Initialize();
    log_info("Connected to VM");
    while (true) {
        auto s = c.Receive();
        std::cout << *s << std::endl;
        std::string command;
        std::cout << "> ";
        std::getline(std::cin, command);
        if (!std::cin) {
            break;
        }
        c.Send(command);
        if (command == "CONTINUE") {
            c.Receive();
        }
    }
}
