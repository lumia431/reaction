/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"

// Test reset functionality of reactive calculations
TEST(DependencyGraphTest, TestReset) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);

    auto ds = reaction::calc([](auto aa, auto bb) { return aa + bb; }, a, b);

    auto dds = reaction::calc([](auto aa, auto bb) { return aa + bb; }, a, b);

    dds.reset([&]() { return a() + ds(); });
    a.value(2);
    EXPECT_EQ(dds.get(), 6);
}

// Test detection of self-dependency cycles
TEST(DependencyGraphTest, TestSelfDependency) {
    auto a   = reaction::var(1);
    auto dsA = reaction::calc([](int aa) { return aa; }, a);

    EXPECT_THROW(dsA.reset([&]() { return a() + dsA(); }), std::runtime_error);
}

// Test detection of circular dependencies
TEST(DependencyGraphTest, TestCycleDependency) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);
    auto c = reaction::var(3);

    auto dsA = reaction::calc([](int bb) { return bb; }, b);

    auto dsB = reaction::calc([](int cc) { return cc; }, c);

    auto dsC = reaction::calc([](int aa) { return aa; }, a);

    dsA.reset([&]() { return b() + dsB(); });

    dsB.reset([&]() { return c() * dsC(); });

    EXPECT_THROW(dsC.reset([&]() { return a() - dsA(); }), std::runtime_error);
}