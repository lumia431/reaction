/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"

// Test expression templates with reactive values
TEST(ExpressionTemplatesTest, TestExpr) {
    auto a       = reaction::var(1);
    auto b       = reaction::var(2);
    auto c       = reaction::var(3.14);
    auto ds      = reaction::calc([&]() { return a() + b(); });
    auto expr_ds = reaction::expr(c + a / b - ds * 2);

    a.value(2);
    EXPECT_EQ(ds.get(), 4);
    ASSERT_FLOAT_EQ(expr_ds.get(), -3.86);
}