#include <gtest/gtest.h>
#include "compiler.hpp"

TEST(CompilerTest, GreetingTest) {
    // 测试库中的函数返回是否符合预期
    EXPECT_EQ(kotlin_lite::getGreeting(), "Hello from kotlin-lite library!");
}

TEST(MathTest, Basic) {
    EXPECT_EQ(2 + 2, 4);
}