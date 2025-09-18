/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"

// Test close handle for reactive graph cleanup
TEST(InvalidationTest, TestCloseHandle) {
    auto a = reaction::var(1).setName("a");
    auto b = reaction::var(2).setName("b");

    auto dsB = reaction::calc([](auto aa) { return aa; }, a).setName("dsB");
    auto dsC = reaction::calc([](auto aa) { return aa; }, a).setName("dsC");
    auto dsD = reaction::calc([](auto aa) { return aa; }, a).setName("dsD");
    auto dsE = reaction::calc([](auto aa) { return aa; }, a).setName("dsE");
    auto dsF = reaction::calc([](auto aa) { return aa; }, a).setName("dsF");
    auto dsG = reaction::calc([](auto aa) { return aa; }, a).setName("dsG");

    {
        auto dsA = reaction::calc<reaction::ChangeTrig, reaction::CloseHandle>([](int aa) { return aa; }, a).setName("dsA");
        dsB.reset([&]() { return a() + dsA(); });
        dsC.reset([&]() { return a() + dsA() + dsB(); });
        dsD.reset([&]() { return dsA() + dsB() + dsC(); });
        dsE.reset([&]() { return dsB() * dsC() + dsD(); });
        dsF.reset([&]() { return a() + b(); });
        dsG.reset([&]() { return dsA() + dsF(); });
    }

    EXPECT_FALSE(static_cast<bool>(dsB));
    EXPECT_FALSE(static_cast<bool>(dsC));
    EXPECT_FALSE(static_cast<bool>(dsD));
    EXPECT_FALSE(static_cast<bool>(dsE));
    EXPECT_TRUE(static_cast<bool>(dsF));
    EXPECT_FALSE(static_cast<bool>(dsG));
}

// Test keep handle for maintaining dependencies
TEST(InvalidationTest, TestKeepHandle) {
    auto a = reaction::var(1).setName("a");

    auto dsB = reaction::calc([](auto aa) { return aa; }, a).setName("dsB");
    auto dsC = reaction::calc([](auto aa) { return aa; }, a).setName("dsC");

    {
        auto dsA = reaction::calc([](int aa) { return aa; }, a).setName("dsA");

        dsB.reset([](int aa, int AA) { return aa + AA; }, a, dsA);
        dsC.reset([](int aa, int AA, int BB) { return aa + AA + BB; }, a, dsA, dsB);
    }

    EXPECT_EQ(dsB.get(), 2);
    EXPECT_EQ(dsC.get(), 4);

    a.value(10);
    EXPECT_EQ(dsB.get(), 20);
    EXPECT_EQ(dsC.get(), 40);
}

// Test last handle for maintaining only the last value
TEST(InvalidationTest, TestLastHandle) {
    auto a = reaction::var(1).setName("a");

    auto dsB = reaction::calc([](auto aa) { return aa; }, a).setName("dsB");
    auto dsC = reaction::calc([](auto aa) { return aa; }, a).setName("dsC");

    {
        auto dsA = reaction::calc<reaction::ChangeTrig, reaction::LastHandle>([](int aa) { return aa; }, a).setName("dsA");

        dsB.reset([](int aa, int AA) { return aa + AA; }, a, dsA);
        dsC.reset([](int aa, int AA, int BB) { return aa + AA + BB; }, a, dsA, dsB);
    }

    EXPECT_EQ(dsB.get(), 2);
    EXPECT_EQ(dsC.get(), 4);

    a.value(10);
    EXPECT_EQ(dsB.get(), 11);
    EXPECT_EQ(dsC.get(), 22);
}