/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"
#include <gtest/gtest.h>

// Test reset consistency guarantees
TEST(ResetConsistencyTest, ResetCycleDependencyRollback) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);

    // Create a valid calculation first
    auto calc = reaction::calc([](int x, int y) { return x + y; }, a, b);
    EXPECT_EQ(calc.get(), 3);

    // Change values and verify calculation still works
    a.value(10);
    EXPECT_EQ(calc.get(), 12);

    // Attempt to reset with a cycle dependency (should fail and rollback)
    EXPECT_THROW(calc.reset([&]() { return a() + calc(); }), std::runtime_error);

    // Verify the node is still in a consistent state
    EXPECT_EQ(calc.get(), 12); // Value should be unchanged

    // Verify dependencies are still intact by changing inputs
    a.value(20);
    EXPECT_EQ(calc.get(), 22);
    b.value(5);
    EXPECT_EQ(calc.get(), 25);
}

TEST(ResetConsistencyTest, ResetSelfDependencyRollback) {
    auto a = reaction::var(1);

    // Create a valid calculation
    auto calc = reaction::calc([](int x) { return x * 2; }, a);
    EXPECT_EQ(calc.get(), 2);

    // Attempt to reset with self-dependency (should fail and rollback)
    EXPECT_THROW(calc.reset([&]() { return calc() + 1; }), std::runtime_error);

    // Verify the node is still in a consistent state
    EXPECT_EQ(calc.get(), 2);

    // Verify dependencies are still intact
    a.value(5);
    EXPECT_EQ(calc.get(), 10);
}

TEST(ResetConsistencyTest, ResetWithMultipleDependenciesPartialFailure) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);

    // Create two calculations
    auto calc1 = reaction::calc([](int x) { return x * 2; }, a);
    auto calc2 = reaction::calc([](int y) { return y + 1; }, b);

    // Make calc1 depend on calc2
    calc1.reset([&]() { return calc2() * 2; });
    EXPECT_EQ(calc1.get(), 6); // (2+1) * 2 = 6

    // Now attempt to make calc2 depend on calc1, creating a cycle
    EXPECT_THROW(calc2.reset([&]() { return calc1() + 1; }), std::runtime_error);

    // Verify calc2 is still in a consistent state
    EXPECT_EQ(calc2.get(), 3); // b(2) + 1 = 3

    // Verify calc2's dependencies are still intact
    b.value(10);
    EXPECT_EQ(calc2.get(), 11); // 10 + 1
}

TEST(ResetConsistencyTest, ResetWithExceptionInEvaluationRollback) {
    auto a = reaction::var(1);

    // Create a valid calculation
    auto calc = reaction::calc([](int x) { return x * 2; }, a);
    EXPECT_EQ(calc.get(), 2);

    // Attempt to reset with a function that throws during evaluation
    EXPECT_THROW(calc.reset([&]() {
        if (a() == 1) {
            throw std::runtime_error("Test exception during evaluation");
        }
        return a() * 3;
    }),
        std::runtime_error);

    // Verify the node is still in a consistent state
    EXPECT_EQ(calc.get(), 2);

    // Verify dependencies are still intact
    a.value(5);
    EXPECT_EQ(calc.get(), 10);
}

TEST(ResetConsistencyTest, SuccessfulResetAfterFailedReset) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);

    // Create a valid calculation
    auto calc = reaction::calc([](int x, int y) { return x + y; }, a, b);
    EXPECT_EQ(calc.get(), 3);

    // Attempt a failed reset
    EXPECT_THROW(calc.reset([&]() { return calc() + a(); }), std::runtime_error);

    // Verify still consistent
    EXPECT_EQ(calc.get(), 3);

    // Now do a successful reset
    calc.reset([&]() { return a() * b(); });
    EXPECT_EQ(calc.get(), 2);

    // Verify new calculation works
    a.value(3);
    EXPECT_EQ(calc.get(), 6);
    b.value(4);
    EXPECT_EQ(calc.get(), 12);
}

TEST(ResetConsistencyTest, ResetVoidExpressionConsistency) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);

    // Create calculations that will form a cycle
    auto calc1 = reaction::calc([](int x) { return x; }, a);
    auto calc2 = reaction::calc([](int y) { return y; }, b);

    // First, make calc1 depend on calc2
    calc1.reset([&]() { return calc2() + 1; });
    EXPECT_EQ(calc1.get(), 3); // b(2) + 1 = 3

    // Now attempt to make calc2 depend on calc1, creating a cycle
    EXPECT_THROW(calc2.reset([&]() { return calc1() + 1; }), std::runtime_error);

    // Verify calc2 is still in a consistent state
    EXPECT_EQ(calc2.get(), 2);

    // Verify calc2's dependencies are still intact
    b.value(5);
    EXPECT_EQ(calc2.get(), 5);
}
