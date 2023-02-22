#include <gtest/gtest.h>
#include "t86/os.h"
#include "common/parser.h"
#include "messenger.h"
#include <string>
#include <queue>

class Comms: public Messenger {
public:
    Comms(std::queue<std::string>& in, std::vector<std::string>& out): in(in), out(out) { }
    void Send(const std::string&s) override {
        out.push_back(s);
    }

    std::optional<std::string> Receive() override {
        if (out.empty()) {
            return std::nullopt;
        }
        auto x = in.front();
        in.pop();
        return x;
    }
    std::queue<std::string> in;
    std::vector<std::string> out;
};

using namespace tiny::t86;

TEST(DebugTest, One) {
    OS os;
    std::queue<std::string> in({"REASON", "PEEKREGS IP", "CONTINUE", "REASON", "PEEKREGS IP", "CONTINUE", "CONTINUE"});
    std::vector<std::string> out;

    os.SetDebuggerComms(std::make_unique<Comms>(in, out));

    std::istringstream iss{
R"(
.text

MOV R0, 1
MOV R1, 2
ADD R0, R1
HALT
)"
    };

    Parser parser(iss);
    Program p = parser.Parse();

    os.Run(std::move(p));
    ASSERT_EQ(out[0], "Execution start");
}
