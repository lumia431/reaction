/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"

// Test basic calculation functionality with different types
TEST(BasicOperationsTest, TestCalc) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    EXPECT_EQ(a.get(), 1);
    EXPECT_EQ(b.get(), 3.14);

    auto ds = reaction::calc([](int aa, double bb) { return aa + bb; }, a, b);
    auto dds = reaction::calc([](int aa, double dsds) { return std::to_string(aa) + std::to_string(dsds); }, a, ds);
    auto ddds = reaction::calc([]() { return 1; });

    ASSERT_FLOAT_EQ(ds.get(), 4.14);
    EXPECT_EQ(dds.get(), "14.140000");
    EXPECT_EQ(ddds.get(), 1);
}

// Test reactive value updates and triggering
TEST(BasicOperationsTest, TestTrigger) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    EXPECT_EQ(a.get(), 1);
    EXPECT_EQ(b.get(), 3.14);

    auto ds = reaction::calc([](int aa, double bb) { return aa + bb; }, a, b);
    auto dds = reaction::calc([](auto aa, auto dsds) { return std::to_string(aa) + std::to_string(dsds); }, a, ds);

    ASSERT_FLOAT_EQ(ds.get(), 4.14);
    EXPECT_EQ(dds.get(), "14.140000");

    a.value(2);
    ASSERT_FLOAT_EQ(ds.get(), 5.14);
    EXPECT_EQ(dds.get(), "25.140000");
}

// Test operator() syntax for reactive values
TEST(BasicOperationsTest, TestParentheses) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    EXPECT_EQ(a.get(), 1);
    EXPECT_EQ(b.get(), 3.14);

    auto ds = reaction::calc([&]() { return a() + b(); });
    auto dds = reaction::calc([&]() { return std::to_string(a()) + std::to_string(ds()); });

    ASSERT_FLOAT_EQ(ds.get(), 4.14);
    EXPECT_EQ(dds.get(), "14.140000");

    a.value(2);
    ASSERT_FLOAT_EQ(ds.get(), 5.14);
    EXPECT_EQ(dds.get(), "25.140000");
}

// Test constant reactive variables
TEST(BasicOperationsTest, TestConst) {
    auto a = reaction::var(1);
    auto b = reaction::constVar(3.14);
    auto ds = reaction::calc([](int aa, double bb) { return aa + bb; }, a, b);
    ASSERT_FLOAT_EQ(ds.get(), 4.14);

    a.value(2);
    ASSERT_FLOAT_EQ(ds.get(), 5.14);
    // b.value(4.14); // compile error; constVar cannot be modified
}