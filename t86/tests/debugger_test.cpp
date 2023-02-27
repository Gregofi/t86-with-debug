#include <gtest/gtest.h>
#include "debugger/T86Process.h"
#include "MockMessenger.h"

class MockMessenger : public Messenger {
public:
    void Send(const std::string& message) override {
        sent.push(message);
    }

    std::optional<std::string> Receive() override {
        auto m = sent.front();
        sent.pop();

        if (m.starts_with("PEEKTEXT ")) {
            auto s = std::stoi(m.substr(strlen("PEEKTEXT ")));
            return fmt::format("TEXT:{0} VALUE:{1} at {0}", s, "Dummy Instruction");
        } else if (m.starts_with("PEEKDATA ")) {
            auto s = std::stoi(m.substr(strlen("PEEKDATA ")));
            return fmt::format("DATA:{} VALUE:{}", s, s);
        } else {
            assert(false);
        }
    }
    
    std::queue<std::string> sent;
};

TEST(T86ProcessTest, ReadText) {
    T86Process process(std::make_unique<MockMessenger>());
    
    auto data = process.ReadText(0, 1);
    ASSERT_EQ(data.size(), 1);
    ASSERT_EQ(data.at(0), "Dummy Instruction at 0");

    data = process.ReadText(0, 2);
    ASSERT_EQ(data.size(), 2);
    ASSERT_EQ(data.at(0), "Dummy Instruction at 0");
    ASSERT_EQ(data.at(1), "Dummy Instruction at 1");

    data = process.ReadText(3, 2);
    ASSERT_EQ(data.size(), 2);
    ASSERT_EQ(data.at(0), "Dummy Instruction at 3");
    ASSERT_EQ(data.at(1), "Dummy Instruction at 4");

    data = process.ReadText(3, 0);
    ASSERT_EQ(data.size(), 0);
}

TEST(T86ProcessTest, ReadMemory) {
    T86Process process(std::make_unique<MockMessenger>());

    auto data = process.ReadMemory(0, 1);
    ASSERT_EQ(data.size(), 1);
    ASSERT_EQ(data.at(0), 0);

    data = process.ReadMemory(0, 2);
    ASSERT_EQ(data.size(), 2);
    ASSERT_EQ(data.at(0), 0);
    ASSERT_EQ(data.at(1), 1);

    data = process.ReadMemory(3, 2);
    ASSERT_EQ(data.size(), 2);
    ASSERT_EQ(data.at(0), 3);
    ASSERT_EQ(data.at(1), 4);

    data = process.ReadMemory(3, 0);
    ASSERT_EQ(data.size(), 0);
}
