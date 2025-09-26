/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"

/**
 * @brief Test expression template division operator with floating-point precision
 *
 * This test verifies that the division operator properly handles integer division
 * by promoting to floating-point types to ensure accurate results.
 */
TEST(ExpressionDivisionTest, TestIntegerDivisionPromotion) {
    using namespace reaction;

    // Test basic integer division that should return floating-point result
    auto a = var(1).setName("a");
    auto b = var(2).setName("b");
    auto division = expr(a / b).setName("division");

    EXPECT_DOUBLE_EQ(division.get(), 0.5);
    EXPECT_NE(division.get(), 0); // Ensure it's not integer division

    // Test with larger integers
    a.value(7);
    b.value(3);
    EXPECT_DOUBLE_EQ(division.get(), 7.0 / 3.0);

    // Test division by larger number
    a.value(1);
    b.value(10);
    EXPECT_DOUBLE_EQ(division.get(), 0.1);
}

/**
 * @brief Test mixed-type division (integer and floating-point)
 */
TEST(ExpressionDivisionTest, TestMixedTypeDivision) {
    using namespace reaction;

    auto intVar = var(5).setName("intVar");
    auto floatVar = var(2.0).setName("floatVar");

    // Integer divided by float - mixed type expressions work naturally
    auto intByFloat = calc([&]() { return intVar() / floatVar(); }).setName("intByFloat");
    EXPECT_DOUBLE_EQ(intByFloat.get(), 2.5);

    // Float divided by integer - mixed type expressions work naturally
    auto floatByInt = calc([&]() { return floatVar() / intVar(); }).setName("floatByInt");
    EXPECT_DOUBLE_EQ(floatByInt.get(), 0.4);

    // Update values
    intVar.value(10);
    floatVar.value(4.0);
    EXPECT_DOUBLE_EQ(intByFloat.get(), 2.5);
    EXPECT_DOUBLE_EQ(floatByInt.get(), 0.4);
}

/**
 * @brief Test pure floating-point division (should remain unchanged)
 */
TEST(ExpressionDivisionTest, TestFloatingPointDivision) {
    using namespace reaction;

    auto a = var(3.14).setName("a");
    auto b = var(2.0).setName("b");
    auto division = expr(a / b).setName("division");

    EXPECT_DOUBLE_EQ(division.get(), 1.57);

    // Test with different values
    a.value(7.5);
    b.value(2.5);
    EXPECT_DOUBLE_EQ(division.get(), 3.0);
}

/**
 * @brief Test complex expression chains with division
 */
TEST(ExpressionDivisionTest, TestComplexExpressionChains) {
    using namespace reaction;

    auto a = var(10).setName("a");
    auto b = var(3).setName("b");
    auto c = var(2).setName("c");

    // Test (a / b) + c - using expression templates
    auto complexExpr1 = expr((a / b) + c).setName("complexExpr1");
    EXPECT_DOUBLE_EQ(complexExpr1.get(), 10.0 / 3.0 + 2.0);

    // Test a / (b + c) - using expression templates
    auto complexExpr2 = expr(a / (b + c)).setName("complexExpr2");
    EXPECT_DOUBLE_EQ(complexExpr2.get(), 10.0 / 5.0);

    // Test (a / b) / c - using expression templates
    auto complexExpr3 = expr((a / b) / c).setName("complexExpr3");
    EXPECT_DOUBLE_EQ(complexExpr3.get(), (10.0 / 3.0) / 2.0);

    // Update values and verify recalculation
    a.value(15);
    b.value(4);
    c.value(3);

    EXPECT_DOUBLE_EQ(complexExpr1.get(), 15.0 / 4.0 + 3.0);
    EXPECT_DOUBLE_EQ(complexExpr2.get(), 15.0 / 7.0);
    EXPECT_DOUBLE_EQ(complexExpr3.get(), (15.0 / 4.0) / 3.0);
}

/**
 * @brief Test division with reactive calculations
 */
TEST(ExpressionDivisionTest, TestDivisionWithCalculations) {
    using namespace reaction;

    auto x = var(6).setName("x");
    auto y = var(4).setName("y");

    // Create intermediate calculations
    auto sum = calc([&]() { return x() + y(); }).setName("sum");
    auto diff = calc([&]() { return x() - y(); }).setName("diff");

    // Division of calculations using expression templates
    auto ratio = expr(sum / diff).setName("ratio");
    EXPECT_DOUBLE_EQ(ratio.get(), 10.0 / 2.0); // (6+4) / (6-4) = 10/2 = 5.0

    // Test with different values
    x.value(12);
    y.value(3);
    EXPECT_DOUBLE_EQ(ratio.get(), 15.0 / 9.0); // (12+3) / (12-3) = 15/9
}

/**
 * @brief Test edge cases for division
 */
TEST(ExpressionDivisionTest, TestDivisionEdgeCases) {
    using namespace reaction;

    auto a = var(0).setName("a");
    auto b = var(5).setName("b");

    // Zero divided by number using expression templates
    auto zeroDiv = expr(a / b).setName("zeroDiv");
    EXPECT_DOUBLE_EQ(zeroDiv.get(), 0.0);

    // Test very small integer division
    a.value(1);
    b.value(1000);
    EXPECT_DOUBLE_EQ(zeroDiv.get(), 0.001);

    // Test negative numbers
    a.value(-10);
    b.value(3);
    EXPECT_DOUBLE_EQ(zeroDiv.get(), -10.0 / 3.0);

    a.value(10);
    b.value(-4);
    EXPECT_DOUBLE_EQ(zeroDiv.get(), -2.5);
}

/**
 * @brief Test division in batch operations
 */
TEST(ExpressionDivisionTest, TestDivisionInBatchOperations) {
    using namespace reaction;

    auto numerator = var(15).setName("numerator");
    auto denominator = var(4).setName("denominator");
    auto result = expr(numerator / denominator).setName("result");

    int triggerCount = 0;
    auto observer = action([&](double value) {
        triggerCount++;
        EXPECT_DOUBLE_EQ(value, 3.75); // 15/4 = 3.75
    },
        result);

    triggerCount = 0;

    // Test batch execution
    batchExecute([&]() {
        numerator.value(15);
        denominator.value(4);
    });

    EXPECT_EQ(triggerCount, 1); // Should trigger only once
    EXPECT_DOUBLE_EQ(result.get(), 3.75);
}