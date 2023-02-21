#include <gtest/gtest.h>
#include "common/helpers.h"

TEST(SplitTest, All) {
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

TEST(svtoi64, All) {
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

TEST(merge_views, All) {
    auto s = {"Hello", "World"};
    auto res = utils::join(s.begin(), s.end());
    ASSERT_EQ(res, "Hello World");

    s = {"Hello"};
    res = utils::join(s.begin(), s.end());
    ASSERT_EQ(res, "Hello");

    s = {};
    res = utils::join(s.begin(), s.end());
    ASSERT_EQ(res, "");
}
