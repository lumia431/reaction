/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"

// Test with SBO enabled
#define REACTION_ENABLE_SBO 1
#include "../../include/reaction/reaction.h"

// Small test structure
struct SmallStruct {
    int x, y;
    float z;
    
    SmallStruct(int x = 0, int y = 0, float z = 0.0f) : x(x), y(y), z(z) {}
    
    bool operator==(const SmallStruct& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
    
    bool operator!=(const SmallStruct& other) const {
        return !(*this == other);
    }
};

// Large test structure that exceeds SBO threshold
struct LargeStruct {
    std::array<double, 20> data;
    std::string description;
    
    LargeStruct() : data{}, description("test") {}
    
    bool operator==(const LargeStruct& other) const {
        return data == other.data && description == other.description;
    }
    
    bool operator!=(const LargeStruct& other) const {
        return !(*this == other);
    }
};

class SBOOptimizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure clean state
    }
    
    void TearDown() override {
        // Clean up
    }
};

// Test SBO configuration and type detection
TEST_F(SBOOptimizationTest, SBOConfiguration) {
    // Check that SBO is enabled
    EXPECT_TRUE(reaction::ResourceInfo::isSBOEnabled());
    
    // Check SBO threshold
    EXPECT_EQ(reaction::SBO_THRESHOLD, 32);
    
    // Check storage strategies for different types
    EXPECT_STREQ(reaction::ResourceInfo::getStorageStrategy<int>(), "stack (SBO)");
    EXPECT_STREQ(reaction::ResourceInfo::getStorageStrategy<double>(), "stack (SBO)");
    EXPECT_STREQ(reaction::ResourceInfo::getStorageStrategy<SmallStruct>(), "stack (SBO)");
    EXPECT_STREQ(reaction::ResourceInfo::getStorageStrategy<LargeStruct>(), "heap");
    EXPECT_STREQ(reaction::ResourceInfo::getStorageStrategy<std::string>(), "heap");
}

// Test basic SBO functionality with small types
TEST_F(SBOOptimizationTest, SmallTypesUseSBO) {
    // Create reactive variables with small types
    auto int_var = reaction::var(42);
    auto double_var = reaction::var(3.14);
    auto small_var = reaction::var(SmallStruct{1, 2, 3.5f});
    
    // Verify they use SBO
    EXPECT_TRUE(decltype(int_var)::react_type::isUsingSBO());
    EXPECT_TRUE(decltype(double_var)::react_type::isUsingSBO());
    EXPECT_TRUE(decltype(small_var)::react_type::isUsingSBO());
    
    // Test values are correct
    EXPECT_EQ(int_var.get(), 42);
    EXPECT_DOUBLE_EQ(double_var.get(), 3.14);
    EXPECT_EQ(small_var.get().x, 1);
    EXPECT_EQ(small_var.get().y, 2);
    EXPECT_FLOAT_EQ(small_var.get().z, 3.5f);
}

// Test that large types still use heap allocation
TEST_F(SBOOptimizationTest, LargeTypesUseHeap) {
    auto large_var = reaction::var(LargeStruct{});
    auto string_var = reaction::var(std::string("test"));
    
    // Verify they don't use SBO
    EXPECT_FALSE(decltype(large_var)::react_type::isUsingSBO());
    EXPECT_FALSE(decltype(string_var)::react_type::isUsingSBO());
    
    // Test values are correct
    EXPECT_EQ(large_var.get().description, "test");
    EXPECT_EQ(string_var.get(), "test");
}

// Test value updates with SBO
TEST_F(SBOOptimizationTest, SBOValueUpdates) {
    auto int_var = reaction::var(10);
    auto small_var = reaction::var(SmallStruct{1, 2, 3.0f});
    
    // Update values
    int_var.value(20);
    small_var.value(SmallStruct{4, 5, 6.0f});
    
    // Verify updates
    EXPECT_EQ(int_var.get(), 20);
    EXPECT_EQ(small_var.get().x, 4);
    EXPECT_EQ(small_var.get().y, 5);
    EXPECT_FLOAT_EQ(small_var.get().z, 6.0f);
}

// Test reactive calculations with SBO
TEST_F(SBOOptimizationTest, SBOReactiveCalculations) {
    auto a = reaction::var(5);
    auto b = reaction::var(10);
    
    // Create calculation that should also use SBO
    auto sum = reaction::calc([](int x, int y) { return x + y; }, a, b);
    auto product = reaction::calc([](int x, int y) { return x * y; }, a, b);
    
    // Verify SBO usage
    EXPECT_TRUE(decltype(sum)::react_type::isUsingSBO());
    EXPECT_TRUE(decltype(product)::react_type::isUsingSBO());
    
    // Verify values
    EXPECT_EQ(sum.get(), 15);
    EXPECT_EQ(product.get(), 50);
    
    // Update inputs and verify reactive updates
    a.value(7);
    EXPECT_EQ(sum.get(), 17);
    EXPECT_EQ(product.get(), 70);
}

// Test move semantics with SBO
TEST_F(SBOOptimizationTest, SBOMoveSemantics) {
    auto original = reaction::var(42);
    EXPECT_TRUE(decltype(original)::react_type::isUsingSBO());
    EXPECT_EQ(original.get(), 42);
    
    // Move construct
    auto moved = std::move(original);
    EXPECT_TRUE(decltype(moved)::react_type::isUsingSBO());
    EXPECT_EQ(moved.get(), 42);
}

// Test memory footprint comparison
TEST_F(SBOOptimizationTest, MemoryFootprint) {
    // For small types, SBO should have predictable storage size
    constexpr size_t int_storage = reaction::ResourceInfo::getStorageSize<int>();
    constexpr size_t double_storage = reaction::ResourceInfo::getStorageSize<double>();
    constexpr size_t small_storage = reaction::ResourceInfo::getStorageSize<SmallStruct>();
    
    // SBO storage should be close to the actual type size (plus some overhead)
    EXPECT_GE(int_storage, sizeof(int));
    EXPECT_GE(double_storage, sizeof(double));
    EXPECT_GE(small_storage, sizeof(SmallStruct));
    
    // Should be significantly smaller than heap allocation overhead
    EXPECT_LT(int_storage, sizeof(int) + 64); // Reasonable overhead limit
}

// Test that SBO doesn't break existing functionality
TEST_F(SBOOptimizationTest, BackwardCompatibility) {
    // Test basic reactive patterns still work
    auto counter = reaction::var(0);
    auto doubled = reaction::calc([](int x) { return x * 2; }, counter);
    auto message = reaction::calc([](int x) { 
        return std::string("Count: ") + std::to_string(x); 
    }, doubled);
    
    EXPECT_EQ(counter.get(), 0);
    EXPECT_EQ(doubled.get(), 0);
    EXPECT_EQ(message.get(), "Count: 0");
    
    counter.value(5);
    EXPECT_EQ(counter.get(), 5);
    EXPECT_EQ(doubled.get(), 10);
    EXPECT_EQ(message.get(), "Count: 10");
    
    // Verify SBO is being used where appropriate
    EXPECT_TRUE(decltype(counter)::react_type::isUsingSBO());
    EXPECT_TRUE(decltype(doubled)::react_type::isUsingSBO());
    EXPECT_FALSE(decltype(message)::react_type::isUsingSBO()); // string is too large
}

// Test exception handling with SBO
TEST_F(SBOOptimizationTest, SBOExceptionHandling) {
    // Test that exceptions work correctly with SBO
    auto uninit_var = reaction::var<int>();
    
    // Should throw when accessing uninitialized variable
    EXPECT_THROW(uninit_var.get(), std::runtime_error);
    
    // After initialization, should work normally
    uninit_var.value(42);
    EXPECT_EQ(uninit_var.get(), 42);
}

// Performance hint test (compile-time checks)
TEST_F(SBOOptimizationTest, CompileTimeChecks) {
    // These should be compile-time constant expressions
    static_assert(reaction::should_use_sbo_v<int>);
    static_assert(reaction::should_use_sbo_v<double>);
    static_assert(reaction::should_use_sbo_v<SmallStruct>);
    static_assert(!reaction::should_use_sbo_v<LargeStruct>);
    static_assert(!reaction::should_use_sbo_v<std::string>);
    
    // Storage size should be reasonable
    static_assert(reaction::ResourceInfo::getStorageSize<int>() < 64);
    static_assert(reaction::ResourceInfo::getStorageSize<double>() < 64);
}