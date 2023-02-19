#include "common/TCP.h"
#include "common/logger.h"

int main() {
    TCP::TCPClient c(9110);
    c.Initialize();
    log_info("Connected to VM");

    auto s = c.Receive();
    std::cout << *s << std::endl;
    c.Send("REASON");
    s = c.Receive();
    std::cout << *s << std::endl;
    c.Send("CONT");
    s = c.Receive();
    std::cout << *s << std::endl;
}
