/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"

/**
 * @brief Test comparison operators in expression templates
 *
 * This test verifies that all comparison operators work correctly
 * with reactive values in expression templates.
 */
TEST(ExpressionOperatorsTest, TestComparisonOperators) {
    using namespace reaction;

    auto a = var(5).setName("a");
    auto b = var(3).setName("b");
    auto c = var(5).setName("c");

    // Test equality
    auto eq_expr = expr(a == c).setName("eq_expr");
    EXPECT_TRUE(eq_expr.get());

    auto neq_expr = expr(a == b).setName("neq_expr");
    EXPECT_FALSE(neq_expr.get());

    // Test inequality
    auto ne_expr = expr(a != b).setName("ne_expr");
    EXPECT_TRUE(ne_expr.get());

    auto ne_expr2 = expr(a != c).setName("ne_expr2");
    EXPECT_FALSE(ne_expr2.get());

    // Test less than
    auto lt_expr = expr(b < a).setName("lt_expr");
    EXPECT_TRUE(lt_expr.get());

    auto lt_expr2 = expr(a < b).setName("lt_expr2");
    EXPECT_FALSE(lt_expr2.get());

    // Test greater than
    auto gt_expr = expr(a > b).setName("gt_expr");
    EXPECT_TRUE(gt_expr.get());

    auto gt_expr2 = expr(b > a).setName("gt_expr2");
    EXPECT_FALSE(gt_expr2.get());

    // Test less than or equal
    auto le_expr = expr(b <= a).setName("le_expr");
    EXPECT_TRUE(le_expr.get());

    auto le_expr2 = expr(a <= c).setName("le_expr2");
    EXPECT_TRUE(le_expr2.get());

    // Test greater than or equal
    auto ge_expr = expr(a >= b).setName("ge_expr");
    EXPECT_TRUE(ge_expr.get());

    auto ge_expr2 = expr(a >= c).setName("ge_expr2");
    EXPECT_TRUE(ge_expr2.get());

    // Test dynamic updates
    b.value(7);
    EXPECT_FALSE(lt_expr.get()); // 7 < 5 is false
    EXPECT_TRUE(gt_expr2.get()); // 7 > 5 is true
}

/**
 * @brief Test logical operators in expression templates
 */
TEST(ExpressionOperatorsTest, TestLogicalOperators) {
    using namespace reaction;

    auto a = var(true).setName("a");
    auto b = var(false).setName("b");
    auto c = var(true).setName("c");

    // Test logical AND
    auto and_expr1 = expr(a && c).setName("and_expr1");
    EXPECT_TRUE(and_expr1.get());

    auto and_expr2 = expr(a && b).setName("and_expr2");
    EXPECT_FALSE(and_expr2.get());

    // Test logical OR
    auto or_expr1 = expr(a || b).setName("or_expr1");
    EXPECT_TRUE(or_expr1.get());

    auto or_expr2 = expr(b || b).setName("or_expr2");
    EXPECT_FALSE(or_expr2.get());

    // Test complex logical expressions
    auto complex_expr = expr((a && c) || b).setName("complex_expr");
    EXPECT_TRUE(complex_expr.get());

    // Test dynamic updates
    b.value(true);
    EXPECT_TRUE(and_expr2.get()); // true && true is true
    EXPECT_TRUE(or_expr2.get());  // true || true is true
}

/**
 * @brief Test unary operators in expression templates
 */
TEST(ExpressionOperatorsTest, TestUnaryOperators) {
    using namespace reaction;

    auto a = var(5).setName("a");
    auto b = var(-3).setName("b");
    auto bool_var = var(true).setName("bool_var");
    auto int_var = var(0b1010).setName("int_var"); // 10 in binary

    // Test unary negation
    auto neg_expr1 = expr(-a).setName("neg_expr1");
    EXPECT_EQ(neg_expr1.get(), -5);

    auto neg_expr2 = expr(-b).setName("neg_expr2");
    EXPECT_EQ(neg_expr2.get(), 3);

    // Test logical NOT
    auto not_expr1 = expr(!bool_var).setName("not_expr1");
    EXPECT_FALSE(not_expr1.get());

    auto not_expr2 = expr(!(a == 5)).setName("not_expr2");
    EXPECT_FALSE(not_expr2.get());

    // Test bitwise NOT
    auto bitnot_expr = expr(~int_var).setName("bitnot_expr");
    EXPECT_EQ(bitnot_expr.get(), ~10);

    // Test dynamic updates
    a.value(-10);
    EXPECT_EQ(neg_expr1.get(), 10);

    bool_var.value(false);
    EXPECT_TRUE(not_expr1.get());
}

/**
 * @brief Test complex operator combinations
 */
TEST(ExpressionOperatorsTest, TestComplexOperatorCombinations) {
    using namespace reaction;

    auto x = var(4).setName("x");
    auto y = var(2).setName("y");
    auto z = var(6).setName("z");

    // Test arithmetic with comparisons
    auto complex1 = expr((x + y) > z).setName("complex1");
    EXPECT_FALSE(complex1.get()); // (4+2) > 6 is false

    // Test nested unary operators
    auto complex2 = expr(-(x - y)).setName("complex2");
    EXPECT_EQ(complex2.get(), -2); // -(4-2) = -2

    // Test logical with arithmetic comparisons
    auto complex3 = expr((x > y) && (z / y == 3)).setName("complex3");
    EXPECT_TRUE(complex3.get()); // (4>2) && (6/2==3) is true

    // Test chained comparisons with logical operators
    auto complex4 = expr((x < z) && (y < x) && (x < z)).setName("complex4");
    EXPECT_TRUE(complex4.get()); // 2 < 4 < 6

    // Update values and verify recalculation
    x.value(8);
    EXPECT_TRUE(complex1.get()); // (8+2) > 6 is true
    EXPECT_EQ(complex2.get(), -6); // -(8-2) = -6
}

/**
 * @brief Test operators with different data types
 */
TEST(ExpressionOperatorsTest, TestOperatorsWithDifferentTypes) {
    using namespace reaction;

    auto int_var = var(5).setName("int_var");
    auto float_var = var(3.14).setName("float_var");
    auto bool_var = var(true).setName("bool_var");

    // Test mixed arithmetic and comparison
    auto mixed_expr1 = expr(int_var > float_var).setName("mixed_expr1");
    EXPECT_TRUE(mixed_expr1.get()); // 5 > 3.14 is true

    // Test boolean with arithmetic
    auto mixed_expr2 = expr(bool_var && (int_var > 0)).setName("mixed_expr2");
    EXPECT_TRUE(mixed_expr2.get()); // true && (5>0) is true

    // Test floating point division with comparison
    auto mixed_expr3 = expr((int_var / 2) > 2.0).setName("mixed_expr3");
    EXPECT_TRUE(mixed_expr3.get()); // 2.5 > 2.0 is true (from our division optimization)

    // Update values
    int_var.value(1);
    EXPECT_FALSE(mixed_expr1.get()); // 1 > 3.14 is false
    EXPECT_FALSE(mixed_expr3.get()); // 0.5 > 2.0 is false
}

/**
 * @brief Test operator precedence and associativity
 */
TEST(ExpressionOperatorsTest, TestOperatorPrecedence) {
    using namespace reaction;

    auto a = var(2).setName("a");
    auto b = var(3).setName("b");
    auto c = var(4).setName("c");

    // Test arithmetic precedence with comparison
    auto prec_expr1 = expr(a + b * c > 10).setName("prec_expr1");
    EXPECT_TRUE(prec_expr1.get()); // 2 + (3*4) = 14 > 10 is true

    // Test unary precedence
    auto prec_expr2 = expr(-a + b).setName("prec_expr2");
    EXPECT_EQ(prec_expr2.get(), 1); // (-2) + 3 = 1

    // Test logical precedence
    auto bool1 = var(true).setName("bool1");
    auto bool2 = var(false).setName("bool2");
    auto bool3 = var(true).setName("bool3");

    auto prec_expr3 = expr(bool1 || bool2 && bool3).setName("prec_expr3");
    EXPECT_TRUE(prec_expr3.get()); // true || (false && true) = true || false = true
}

/**
 * @brief Test operators in batch operations
 */
TEST(ExpressionOperatorsTest, TestOperatorsInBatchOperations) {
    using namespace reaction;

    auto x = var(10).setName("x");
    auto y = var(5).setName("y");
    auto result = expr((x > y) && (x / y == 2)).setName("result");

    int triggerCount = 0;
    auto observer = action([&](bool value) {
        triggerCount++;
        EXPECT_TRUE(value); // Should be true for both original and updated values
    }, result);

    triggerCount = 0;

    // Test batch execution with operators
    batchExecute([&]() {
        x.value(20);
        y.value(10);
    });

    EXPECT_EQ(triggerCount, 1); // Should trigger only once
    EXPECT_TRUE(result.get()); // (20>10) && (20/10==2) is true
}