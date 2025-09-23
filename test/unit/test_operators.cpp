/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"
#include <string>
#include <vector>

/**
 * @brief Test increment operators (++ and --)
 */
TEST(OperatorTest, IncrementDecrementOperators) {
    // Test pre-increment
    auto v1 = reaction::var(5);
    EXPECT_EQ(v1.get(), 5);
    
    ++v1;
    EXPECT_EQ(v1.get(), 6);
    
    // Test post-increment
    auto old_val = v1++;
    EXPECT_EQ(v1.get(), 7);
    EXPECT_EQ(old_val, 6);  // Should return the old value
    
    // Test pre-decrement
    --v1;
    EXPECT_EQ(v1.get(), 6);
    
    // Test post-decrement
    auto old_val2 = v1--;
    EXPECT_EQ(v1.get(), 5);
    EXPECT_EQ(old_val2, 6);  // Should return the old value
    
    std::cout << "✅ Increment/Decrement operators test passed" << std::endl;
}

/**
 * @brief Test compound assignment operators (+=, -=, *=, /=)
 */
TEST(OperatorTest, CompoundAssignmentOperators) {
    auto v = reaction::var(10);
    
    // Test +=
    v += 5;
    EXPECT_EQ(v.get(), 15);
    
    // Test -=
    v -= 3;
    EXPECT_EQ(v.get(), 12);
    
    // Test *=
    v *= 2;
    EXPECT_EQ(v.get(), 24);
    
    // Test /=
    v /= 4;
    EXPECT_EQ(v.get(), 6);
    
    // Test %=
    v %= 4;
    EXPECT_EQ(v.get(), 2);
    
    std::cout << "✅ Compound assignment operators test passed" << std::endl;
}

/**
 * @brief Test operators with floating point numbers
 */
TEST(OperatorTest, FloatingPointOperators) {
    auto vf = reaction::var(3.14);
    
    // Test +=
    vf += 1.86;
    EXPECT_DOUBLE_EQ(vf.get(), 5.0);
    
    // Test -=
    vf -= 1.0;
    EXPECT_DOUBLE_EQ(vf.get(), 4.0);
    
    // Test *=
    vf *= 0.5;
    EXPECT_DOUBLE_EQ(vf.get(), 2.0);
    
    // Test /=
    vf /= 2.0;
    EXPECT_DOUBLE_EQ(vf.get(), 1.0);
    
    // Test increment operators
    ++vf;
    EXPECT_DOUBLE_EQ(vf.get(), 2.0);
    
    auto old_vf = vf++;
    EXPECT_DOUBLE_EQ(vf.get(), 3.0);
    EXPECT_DOUBLE_EQ(old_vf, 2.0);
    
    std::cout << "✅ Floating point operators test passed" << std::endl;
}

/**
 * @brief Test bitwise operators with integers
 */
TEST(OperatorTest, BitwiseOperators) {
    auto v = reaction::var(0b1010);  // 10 in binary
    
    // Test &=
    v &= 0b1100;  // 12 in binary
    EXPECT_EQ(v.get(), 0b1000);  // 8 in binary (1010 & 1100 = 1000)
    
    v.value(0b1010);  // Reset to 10
    
    // Test |=
    v |= 0b0101;  // 5 in binary
    EXPECT_EQ(v.get(), 0b1111);  // 15 in binary (1010 | 0101 = 1111)
    
    v.value(0b1010);  // Reset to 10
    
    // Test ^=
    v ^= 0b0110;  // 6 in binary
    EXPECT_EQ(v.get(), 0b1100);  // 12 in binary (1010 ^ 0110 = 1100)
    
    // Test <<=
    v <<= 1;
    EXPECT_EQ(v.get(), 0b11000);  // 24 in binary (1100 << 1 = 11000)
    
    // Test >>=
    v >>= 2;
    EXPECT_EQ(v.get(), 0b110);  // 6 in binary (11000 >> 2 = 110)
    
    std::cout << "✅ Bitwise operators test passed" << std::endl;
}

/**
 * @brief Test operators with string type
 */
TEST(OperatorTest, StringOperators) {
    auto str = reaction::var(std::string("Hello"));
    
    // Test += with string
    str += " World";
    EXPECT_EQ(str.get(), "Hello World");
    
    str += "!";
    EXPECT_EQ(str.get(), "Hello World!");
    
    std::cout << "✅ String operators test passed" << std::endl;
}

/**
 * @brief Test that operators work with reactive notifications
 */
TEST(OperatorTest, ReactiveNotifications) {
    auto v = reaction::var(0);
    int notification_count = 0;
    
    // Create an action that gets triggered when v changes
    auto action = reaction::action([&notification_count, &v]() {
        notification_count++;
        std::cout << "Value changed to: " << v() << std::endl;
    });
    
    // Initial notification when action is created
    EXPECT_EQ(notification_count, 1);
    
    // Test that operators trigger notifications
    ++v;
    EXPECT_EQ(notification_count, 2);
    EXPECT_EQ(v.get(), 1);
    
    v += 5;
    EXPECT_EQ(notification_count, 3);
    EXPECT_EQ(v.get(), 6);
    
    v *= 2;
    EXPECT_EQ(notification_count, 4);
    EXPECT_EQ(v.get(), 12);
    
    auto old_val = v++;
    EXPECT_EQ(notification_count, 5);
    EXPECT_EQ(v.get(), 13);
    EXPECT_EQ(old_val, 12);
    
    std::cout << "✅ Reactive notifications test passed" << std::endl;
}

/**
 * @brief Test type safety - ensure operators only work on VarExpr
 */
TEST(OperatorTest, TypeSafety) {
    auto var_v = reaction::var(10);
    auto calc_v = reaction::calc([&]() { return var_v() * 2; });
    
    // This should compile and work (VarExpr)
    var_v += 5;
    EXPECT_EQ(var_v.get(), 15);
    
    // These should not compile - CalcExpr cannot be modified with operators
    // Uncomment to test compilation errors:
    // calc_v += 5;  // Should not compile
    // ++calc_v;     // Should not compile
    
    std::cout << "✅ Type safety test passed" << std::endl;
}

/**
 * @brief Test operators with different numeric types
 */
TEST(OperatorTest, MixedNumericTypes) {
    auto v_int = reaction::var(10);
    auto v_double = reaction::var(3.5);
    
    // Test mixing int with double
    v_int += 2.5;  // Should work if underlying type supports it
    // Note: This depends on whether int supports += with double
    // In practice, this would truncate to int
    
    v_double += 1;  // Adding int to double should work
    EXPECT_DOUBLE_EQ(v_double.get(), 4.5);
    
    v_double *= 2;
    EXPECT_DOUBLE_EQ(v_double.get(), 9.0);
    
    std::cout << "✅ Mixed numeric types test passed" << std::endl;
}

/**
 * @brief Test edge cases and boundary conditions
 */
TEST(OperatorTest, EdgeCases) {
    // Test with zero
    auto v_zero = reaction::var(0);
    ++v_zero;
    EXPECT_EQ(v_zero.get(), 1);
    
    --v_zero;
    EXPECT_EQ(v_zero.get(), 0);
    
    // Test with negative numbers
    auto v_neg = reaction::var(-5);
    v_neg += 10;
    EXPECT_EQ(v_neg.get(), 5);
    
    v_neg *= -1;
    EXPECT_EQ(v_neg.get(), -5);
    
    // Test post-increment/decrement return values
    auto v_test = reaction::var(100);
    auto old1 = v_test++;
    auto old2 = v_test--;
    
    EXPECT_EQ(old1, 100);  // Original value
    EXPECT_EQ(old2, 101);  // Value after first increment
    EXPECT_EQ(v_test.get(), 100);  // Back to original after decrement
    
    std::cout << "✅ Edge cases test passed" << std::endl;
}

/**
 * @brief Performance test for operators
 */
TEST(OperatorTest, PerformanceTest) {
    auto v = reaction::var(0);
    auto start = std::chrono::high_resolution_clock::now();
    
    // Perform many operations
    for (int i = 0; i < 10000; ++i) {
        v += 1;
        if (i % 100 == 0) {
            ++v;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_EQ(v.get(), 10000 + 100);  // 10000 additions + 100 increments
    
    std::cout << "✅ Performance test completed in " << duration.count() << " microseconds" << std::endl;
    std::cout << "Final value: " << v.get() << std::endl;
}