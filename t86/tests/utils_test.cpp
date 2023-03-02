#include <gtest/gtest.h>
#include "common/helpers.h"
#include "common/threads_messenger.h"

TEST(Utils, Split) {
    std::string_view s1 = "X Y Z";
    auto v1 = utils::split(s1, ' ');

    ASSERT_EQ(v1.size(), 3);
    ASSERT_EQ(v1.at(0), "X");
    ASSERT_EQ(v1.at(1), "Y");
    ASSERT_EQ(v1.at(2), "Z");

    std::string_view s2 = "X";
    auto v2 = utils::split(s2, ' ');

    ASSERT_EQ(v2.size(), 1);
    ASSERT_EQ(v2.at(0), "X");

    std::string_view s3 = "";
    auto v3 = utils::split(s3, ' ');

    ASSERT_EQ(v3.size(), 0);

    std::string_view s4 = " A B ";
    auto v4 = utils::split(s4, ' ');

    ASSERT_EQ(v4.size(), 2);
    ASSERT_EQ(v4.at(0), "A");
    ASSERT_EQ(v4.at(1), "B");

    std::string_view s5 = "   A    ";
    auto v5 = utils::split(s5, ' ');
    ASSERT_EQ(v5.size(), 1);
    ASSERT_EQ(v5.at(0), "A");
}

TEST(Utils, svtoi64) {
    std::string_view s = "1";
    auto res = utils::svtoi64(s);
    ASSERT_TRUE(res);
    ASSERT_EQ(*res, 1);

    s = "0";
    res = utils::svtoi64(s);
    ASSERT_TRUE(res);
    ASSERT_EQ(res, 0);

    s = "R0";
    res = utils::svtoi64(s);
    ASSERT_FALSE(res);
}

TEST(Utils, join) {
    std::vector<std::string> s = {"Hello", "World"};
    auto res = utils::join(s.begin(), s.end());
    ASSERT_EQ(res, "Hello World");

    s = {"Hello"};
    res = utils::join(s.begin(), s.end());
    ASSERT_EQ(res, "Hello");

    s = {};
    res = utils::join(s.begin(), s.end());
    ASSERT_EQ(res, "");
}

TEST(Utils, is_prefix_of) {
    ASSERT_TRUE(utils::is_prefix_of("Hello", "Hello"));
    ASSERT_TRUE(utils::is_prefix_of("Hell", "Hello"));
    ASSERT_TRUE(utils::is_prefix_of("Hel", "Hello"));
    ASSERT_TRUE(utils::is_prefix_of("He", "Hello"));
    ASSERT_TRUE(utils::is_prefix_of("H", "Hello"));
    ASSERT_TRUE(utils::is_prefix_of("", "Hello"));
    ASSERT_FALSE(utils::is_prefix_of("G", "Hello"));
    ASSERT_FALSE(utils::is_prefix_of("Helg", "Hello"));
    ASSERT_FALSE(utils::is_prefix_of("Hello!", "Hello"));
}

TEST(Utils, squash_strip_whitespace) {
    ASSERT_EQ(utils::squash_strip_whitespace(" A "), "A");
    ASSERT_EQ(utils::squash_strip_whitespace("A "), "A");
    ASSERT_EQ(utils::squash_strip_whitespace(" A"), "A");
    ASSERT_EQ(utils::squash_strip_whitespace("A"), "A");
    ASSERT_EQ(utils::squash_strip_whitespace("A B C"), "A B C");
    ASSERT_EQ(utils::squash_strip_whitespace("A    B C"), "A B C");
    ASSERT_EQ(utils::squash_strip_whitespace("A    B     C"), "A B C");
    ASSERT_EQ(utils::squash_strip_whitespace("  A    B     C"), "A B C");
    ASSERT_EQ(utils::squash_strip_whitespace(""), "");
    ASSERT_EQ(utils::squash_strip_whitespace(" "), "");
}

using MessagesT = std::vector<std::pair<std::vector<std::string>, std::vector<std::string>>>;

/// Contains vector of vector of messages. 
/// For each vector, sends all messages and then calls receive
/// For example, for { {"A", "B"}, {"C"} }, calls twice
/// Send with "A" and "B", then one Receive, then
/// Send with "C", and then final Receive.
void RunThread(ThreadMessenger m, MessagesT messages) {
    for (const auto& [msgs, responses]: messages) {
        for (const auto& message: msgs) {
             m.Send(message);
        }
        for (const auto& response: responses) {
            ASSERT_EQ(*m.Receive(), response);
        }
    }
}

TEST(ThreadMessenger, OneThreadNoWaiting) {
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;

    ThreadMessenger m1(q1, q2);
    ThreadMessenger m2(q2, q1);
    
    m1.Send("Hello");
    m1.Send(" ,");
    m1.Send("World!");
    ASSERT_EQ(*m2.Receive(), "Hello");
    ASSERT_EQ(*m2.Receive(), " ,");
    ASSERT_EQ(*m2.Receive(), "World!");
}

TEST(ThreadMessenger, SimpleWaiting) {
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;

    ThreadMessenger s1(q1, q2);
    ThreadMessenger s2(q2, q1);

    MessagesT m1({{{"Hello", "World"}, {"Thanks!"}}});
    MessagesT m2({{{"Thanks!"}, {"Hello", "World"}}});
    std::thread t1(RunThread, s1, m1);
    std::thread t2(RunThread, s2, m2);
    t1.join();
    t2.join();
}

TEST(ThreadMessenger, MultipleMessages) {
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;

    ThreadMessenger s1(q1, q2);
    ThreadMessenger s2(q2, q1);

    MessagesT m1({
            {{"A"}, {"B"}},
            {{}, {"C"}}
        });
    MessagesT m2({
            {{"B"}, {"A"}},
            {{"C"}, {}}
        });
    std::thread t1(RunThread, s1, m1);
    std::thread t2(RunThread, s2, m2);
    t1.join();
    t2.join();
}
