/*
 * Comprehensive Stress Testing Suite for Reaction Framework
 * 
 * This file contains extensive stress tests designed to find bugs,
 * edge cases, and potential issues in the reaction framework.
 * 
 * Test Categories:
 * 1. Memory Management & Lifecycle
 * 2. Thread Safety & Concurrency
 * 3. Edge Cases & Boundary Conditions
 * 4. Exception Handling
 * 5. Performance & Scalability
 * 6. Complex Scenarios
 * 7. Type System
 * 8. Real-world Usage Patterns
 */

#include "reaction/reaction.h"
#include "gtest/gtest.h"
#include <chrono>
#include <thread>
#include <future>
#include <random>
#include <vector>
#include <memory>
#include <atomic>
#include <limits>
#include <string>
#include <sstream>
#include <exception>
#include <functional>

using namespace reaction;

// =============================================================================
// 1. MEMORY MANAGEMENT & LIFECYCLE TESTS
// =============================================================================

class StressTestMemory : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset any global state if needed
    }
    
    void TearDown() override {
        // Clean up any remaining state
    }
};

// Test massive node creation and destruction
TEST_F(StressTestMemory, MassiveNodeCreationDestruction) {
    constexpr int NODE_COUNT = 10000;
    std::vector<Var<int>> nodes;
    nodes.reserve(NODE_COUNT);
    
    // Create massive amount of nodes
    for (int i = 0; i < NODE_COUNT; ++i) {
        nodes.push_back(var(i));
    }
    
    // Verify all nodes are valid
    for (int i = 0; i < NODE_COUNT; ++i) {
        EXPECT_TRUE(static_cast<bool>(nodes[i]));
        EXPECT_EQ(nodes[i].get(), i);
    }
    
    // Test random access patterns
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, NODE_COUNT - 1);
    
    for (int i = 0; i < 1000; ++i) {
        int idx = dis(gen);
        nodes[idx].value(idx * 2);
        EXPECT_EQ(nodes[idx].get(), idx * 2);
    }
    
    // Nodes will be destroyed when vector goes out of scope
    // This tests proper cleanup of weak references
}

// Test circular reference handling
TEST_F(StressTestMemory, CircularReferenceHandling) {
    auto a = var(1);
    auto b = var(2);
    auto c = var(3);
    
    // Create complex dependency chain
    auto ab = calc([&]() { return a() + b(); });
    auto bc = calc([&]() { return b() + c(); });
    auto ca = calc([&]() { return c() + a(); });
    
    // This should work fine - no actual circular dependency
    auto result = calc([&]() { return ab() + bc() + ca(); });
    EXPECT_EQ(result.get(), 12); // (1+2) + (2+3) + (3+1) = 12
    
    // Try to create actual circular dependency - should throw
    EXPECT_THROW({
        ab.reset([&]() { return bc() + ca(); }); // ab now depends on bc and ca
        bc.reset([&]() { return ca() + ab(); }); // This should create a cycle
    }, std::runtime_error);
}

// Test weak reference lifecycle edge cases
TEST_F(StressTestMemory, WeakReferenceLifecycle) {
    std::vector<Calc<int>> calculations;
    
    {
        auto temp_var = var(42);
        
        // Create calculations that depend on temp_var
        for (int i = 0; i < 100; ++i) {
            calculations.push_back(calc([i](int x) { return x + i; }, temp_var));
        }
        
        // Verify all calculations work
        for (int i = 0; i < 100; ++i) {
            EXPECT_EQ(calculations[i].get(), 42 + i);
        }
        
        // temp_var goes out of scope here
    }
    
    // Calculations should still work (default KeepStrategy)
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(static_cast<bool>(calculations[i]));
        EXPECT_EQ(calculations[i].get(), 42 + i);
    }
}

// Test field-based object lifecycle
TEST_F(StressTestMemory, FieldObjectLifecycle) {
    class TestObject : public FieldBase {
    public:
        TestObject(int val) : m_field(field(val)) {}
        
        void setValue(int val) { m_field.value(val); }
        int getValue() const { return m_field.get(); }
        
    private:
        Var<int> m_field;
    };
    
    std::vector<Var<TestObject>> objects;
    std::vector<Calc<int>> watchers;
    
    // Create objects with field watchers
    for (int i = 0; i < 100; ++i) {
        objects.push_back(var(TestObject(i)));
        watchers.push_back(calc([i](const TestObject& obj) {
            return obj.getValue() * 2;
        }, objects[i]));
    }
    
    // Modify objects and verify watchers update
    for (int i = 0; i < 100; ++i) {
        objects[i]->setValue(i * 10);
        EXPECT_EQ(watchers[i].get(), i * 20);
    }
    
    // Test object replacement
    for (int i = 0; i < 100; ++i) {
        objects[i].value(TestObject(i * 100));
        EXPECT_EQ(watchers[i].get(), i * 200);
    }
}

// Test memory usage with deep dependency chains
TEST_F(StressTestMemory, DeepDependencyChainMemory) {
    constexpr int DEPTH = 1000;
    
    auto root = var(1);
    std::vector<Calc<int>> chain;
    chain.reserve(DEPTH);
    
    // Create deep dependency chain
    chain.push_back(calc([](int x) { return x + 1; }, root));
    for (int i = 1; i < DEPTH; ++i) {
        chain.push_back(calc([](int x) { return x + 1; }, chain[i-1]));
    }
    
    // Verify chain works
    EXPECT_EQ(chain[DEPTH-1].get(), DEPTH + 1);
    
    // Update root and verify propagation
    root.value(100);
    EXPECT_EQ(chain[DEPTH-1].get(), 100 + DEPTH);
    
    // Test batch update
    batchExecute([&]() {
        root.value(200);
    });
    EXPECT_EQ(chain[DEPTH-1].get(), 200 + DEPTH);
}

// Test strategy-based cleanup behavior
TEST_F(StressTestMemory, StrategyBasedCleanup) {
    // Test CloseStrategy
    {
        std::vector<Calc<int, CloseStra>> close_calcs;
        std::vector<Calc<int, KeepStra>> keep_calcs;
        std::vector<Calc<int, LastStra>> last_calcs;
        
        {
            auto temp = var(42);
            
            // Create calculations with different strategies
            for (int i = 0; i < 10; ++i) {
                close_calcs.push_back(calc<ChangeTrig, CloseStra>([i](int x) { return x + i; }, temp));
                keep_calcs.push_back(calc<ChangeTrig, KeepStra>([i](int x) { return x + i; }, temp));
                last_calcs.push_back(calc<ChangeTrig, LastStra>([i](int x) { return x + i; }, temp));
            }
            
            // All should work initially
            for (int i = 0; i < 10; ++i) {
                EXPECT_TRUE(static_cast<bool>(close_calcs[i]));
                EXPECT_TRUE(static_cast<bool>(keep_calcs[i]));
                EXPECT_TRUE(static_cast<bool>(last_calcs[i]));
            }
            
            // temp goes out of scope
        }
        
        // Check strategy behavior
        for (int i = 0; i < 10; ++i) {
            EXPECT_FALSE(static_cast<bool>(close_calcs[i])); // Should be closed
            EXPECT_TRUE(static_cast<bool>(keep_calcs[i]));   // Should remain valid
            EXPECT_TRUE(static_cast<bool>(last_calcs[i]));   // Should remain valid
        }
    }
}

// =============================================================================
// 2. THREAD SAFETY & CONCURRENCY TESTS
// =============================================================================

class StressTestConcurrency : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test concurrent node creation and modification
TEST_F(StressTestConcurrency, ConcurrentNodeCreation) {
    constexpr int THREAD_COUNT = 8;
    constexpr int NODES_PER_THREAD = 1000;
    
    std::vector<std::future<void>> futures;
    std::atomic<int> error_count{0};
    
    // Launch multiple threads creating nodes concurrently
    for (int t = 0; t < THREAD_COUNT; ++t) {
        futures.push_back(std::async(std::launch::async, [t, &error_count]() {
            try {
                std::vector<Var<int>> local_nodes;
                local_nodes.reserve(NODES_PER_THREAD);
                
                // Create nodes
                for (int i = 0; i < NODES_PER_THREAD; ++i) {
                    local_nodes.push_back(var(t * NODES_PER_THREAD + i));
                }
                
                // Modify nodes
                for (int i = 0; i < NODES_PER_THREAD; ++i) {
                    local_nodes[i].value(local_nodes[i].get() * 2);
                }
                
                // Verify values
                for (int i = 0; i < NODES_PER_THREAD; ++i) {
                    int expected = (t * NODES_PER_THREAD + i) * 2;
                    if (local_nodes[i].get() != expected) {
                        error_count++;
                    }
                }
            } catch (...) {
                error_count++;
            }
        }));
    }
    
    // Wait for all threads
    for (auto& future : futures) {
        future.wait();
    }
    
    EXPECT_EQ(error_count.load(), 0);
}

// Test concurrent dependency graph modifications
TEST_F(StressTestConcurrency, ConcurrentDependencyModification) {
    auto shared_var = var(0);
    std::vector<std::future<void>> futures;
    std::atomic<int> error_count{0};
    constexpr int THREAD_COUNT = 4;
    
    for (int t = 0; t < THREAD_COUNT; ++t) {
        futures.push_back(std::async(std::launch::async, [t, &shared_var, &error_count]() {
            try {
                std::vector<Calc<int>> calcs;
                
                // Each thread creates its own calculations depending on shared_var
                for (int i = 0; i < 100; ++i) {
                    calcs.push_back(calc([t, i](int x) { 
                        return x + t * 1000 + i; 
                    }, shared_var));
                }
                
                // Verify calculations
                for (int i = 0; i < 100; ++i) {
                    int expected = shared_var.get() + t * 1000 + i;
                    if (calcs[i].get() != expected) {
                        error_count++;
                    }
                }
                
                // Reset some calculations
                for (int i = 0; i < 50; ++i) {
                    calcs[i].reset([t, i](int x) { 
                        return x * 2 + t * 1000 + i; 
                    }, shared_var);
                }
                
            } catch (...) {
                error_count++;
            }
        }));
    }
    
    // Concurrently modify the shared variable
    std::thread modifier([&shared_var]() {
        for (int i = 0; i < 100; ++i) {
            shared_var.value(i);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });
    
    for (auto& future : futures) {
        future.wait();
    }
    modifier.join();
    
    EXPECT_EQ(error_count.load(), 0);
}

// Test concurrent batch operations
TEST_F(StressTestConcurrency, ConcurrentBatchOperations) {
    constexpr int THREAD_COUNT = 6;
    std::vector<Var<int>> shared_vars;
    
    // Create shared variables
    for (int i = 0; i < 100; ++i) {
        shared_vars.push_back(var(i));
    }
    
    // Create observers
    std::vector<Calc<int>> observers;
    for (int i = 0; i < 100; ++i) {
        observers.push_back(calc([i](int x) { return x * 2; }, shared_vars[i]));
    }
    
    std::vector<std::future<void>> futures;
    std::atomic<int> error_count{0};
    
    // Launch concurrent batch operations
    for (int t = 0; t < THREAD_COUNT; ++t) {
        futures.push_back(std::async(std::launch::async, [t, &shared_vars, &observers, &error_count]() {
            try {
                for (int iteration = 0; iteration < 50; ++iteration) {
                    batchExecute([&]() {
                        // Each thread modifies different subset of variables
                        int start = (t * 100) / THREAD_COUNT;
                        int end = ((t + 1) * 100) / THREAD_COUNT;
                        
                        for (int i = start; i < end; ++i) {
                            shared_vars[i].value(shared_vars[i].get() + 1);
                        }
                    });
                    
                    // Verify observers updated correctly
                    int start = (t * 100) / THREAD_COUNT;
                    int end = ((t + 1) * 100) / THREAD_COUNT;
                    
                    for (int i = start; i < end; ++i) {
                        int expected = shared_vars[i].get() * 2;
                        if (observers[i].get() != expected) {
                            error_count++;
                        }
                    }
                }
            } catch (...) {
                error_count++;
            }
        }));
    }
    
    for (auto& future : futures) {
        future.wait();
    }
    
    EXPECT_EQ(error_count.load(), 0);
}

// Test race conditions in observer notifications
TEST_F(StressTestConcurrency, ObserverNotificationRaceConditions) {
    auto source = var(0);
    std::atomic<int> notification_count{0};
    std::vector<Action<>> actions;
    
    // Create many actions observing the same source
    for (int i = 0; i < 100; ++i) {
        actions.push_back(action([&notification_count, i](int value) {
            notification_count++;
            // Simulate some work
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }, source));
    }
    
    std::vector<std::future<void>> futures;
    
    // Multiple threads updating the source concurrently
    for (int t = 0; t < 4; ++t) {
        futures.push_back(std::async(std::launch::async, [t, &source]() {
            for (int i = 0; i < 25; ++i) {
                source.value(t * 100 + i);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }));
    }
    
    for (auto& future : futures) {
        future.wait();
    }
    
    // Give time for all notifications to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Each update should trigger all 100 actions
    // With 4 threads * 25 updates = 100 updates total
    // So we expect 100 * 100 = 10000 notifications
    EXPECT_EQ(notification_count.load(), 10000);
}

// =============================================================================
// 3. EDGE CASES & BOUNDARY CONDITIONS
// =============================================================================

class StressTestEdgeCases : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test null and empty value handling
TEST_F(StressTestEdgeCases, NullAndEmptyValues) {
    // Test with nullable types
    auto nullable_int = var<std::optional<int>>(std::nullopt);
    EXPECT_FALSE(nullable_int.get().has_value());
    
    auto nullable_calc = calc([](const std::optional<int>& opt) {
        return opt.has_value() ? opt.value() * 2 : -1;
    }, nullable_int);
    
    EXPECT_EQ(nullable_calc.get(), -1);
    
    nullable_int.value(42);
    EXPECT_EQ(nullable_calc.get(), 84);
    
    nullable_int.value(std::nullopt);
    EXPECT_EQ(nullable_calc.get(), -1);
    
    // Test with empty strings
    auto empty_string = var(std::string{});
    auto string_calc = calc([](const std::string& s) {
        return s.empty() ? "empty" : s;
    }, empty_string);
    
    EXPECT_EQ(string_calc.get(), "empty");
    
    empty_string.value("not empty");
    EXPECT_EQ(string_calc.get(), "not empty");
    
    empty_string.value("");
    EXPECT_EQ(string_calc.get(), "empty");
}

// Test extreme numeric values
TEST_F(StressTestEdgeCases, ExtremeNumericValues) {
    // Test with maximum values
    auto max_int = var(std::numeric_limits<int>::max());
    auto max_calc = calc([](int x) { 
        // This might overflow, but should not crash
        return x + 1; 
    }, max_int);
    
    // The result is undefined due to overflow, but should not crash
    EXPECT_NO_THROW(max_calc.get());
    
    // Test with minimum values
    auto min_int = var(std::numeric_limits<int>::min());
    auto min_calc = calc([](int x) { 
        return x - 1; 
    }, min_int);
    
    EXPECT_NO_THROW(min_calc.get());
    
    // Test with floating point extremes
    auto max_double = var(std::numeric_limits<double>::max());
    auto inf_double = var(std::numeric_limits<double>::infinity());
    auto nan_double = var(std::numeric_limits<double>::quiet_NaN());
    
    auto float_calc = calc([](double a, double b, double c) {
        return a + b + c;
    }, max_double, inf_double, nan_double);
    
    EXPECT_NO_THROW(float_calc.get());
    // Result should be NaN due to NaN propagation
    EXPECT_TRUE(std::isnan(float_calc.get()));
}

// Test very deep nesting
TEST_F(StressTestEdgeCases, VeryDeepNesting) {
    constexpr int MAX_DEPTH = 10000;
    
    // Create extremely deep expression nesting
    auto base = var(1.0);
    auto current = base;
    
    // This creates a very deep dependency chain
    for (int i = 0; i < MAX_DEPTH; ++i) {
        // Use expr to create nested expressions
        auto next = expr(current + 0.001);
        current = next;
    }
    
    // This should not cause stack overflow
    EXPECT_NO_THROW({
        double result = current.get();
        EXPECT_NEAR(result, 1.0 + MAX_DEPTH * 0.001, 0.1);
    });
    
    // Test update propagation through deep chain
    EXPECT_NO_THROW({
        base.value(2.0);
        double result = current.get();
        EXPECT_NEAR(result, 2.0 + MAX_DEPTH * 0.001, 0.1);
    });
}

// Test massive expression trees
TEST_F(StressTestEdgeCases, MassiveExpressionTrees) {
    constexpr int WIDTH = 1000;
    
    std::vector<Var<double>> leaves;
    leaves.reserve(WIDTH);
    
    // Create many leaf nodes
    for (int i = 0; i < WIDTH; ++i) {
        leaves.push_back(var(static_cast<double>(i)));
    }
    
    // Create a massive sum expression
    auto sum = expr(leaves[0] + leaves[1]);
    for (int i = 2; i < WIDTH; ++i) {
        sum = expr(sum + leaves[i]);
    }
    
    // Calculate expected sum
    double expected_sum = 0;
    for (int i = 0; i < WIDTH; ++i) {
        expected_sum += i;
    }
    
    EXPECT_NEAR(sum.get(), expected_sum, 0.001);
    
    // Test batch update of all leaves
    batchExecute([&]() {
        for (int i = 0; i < WIDTH; ++i) {
            leaves[i].value(static_cast<double>(i * 2));
        }
    });
    
    EXPECT_NEAR(sum.get(), expected_sum * 2, 0.001);
}

// Test string manipulation edge cases
TEST_F(StressTestEdgeCases, StringManipulationEdgeCases) {
    // Test very long strings
    std::string long_string(100000, 'a');
    auto long_var = var(long_string);
    
    auto concat_calc = calc([](const std::string& s) {
        return s + s; // Double the string
    }, long_var);
    
    EXPECT_EQ(concat_calc.get().length(), 200000);
    
    // Test string with special characters
    std::string special_string = "Hello\n\t\r\0World\xFF\x00";
    auto special_var = var(special_string);
    
    auto special_calc = calc([](const std::string& s) {
        return s.length();
    }, special_var);
    
    EXPECT_EQ(special_calc.get(), special_string.length());
    
    // Test empty string operations
    auto empty_var = var(std::string{});
    auto empty_calc = calc([](const std::string& s) {
        return s + "suffix";
    }, empty_var);
    
    EXPECT_EQ(empty_calc.get(), "suffix");
}

// Test container edge cases
TEST_F(StressTestEdgeCases, ContainerEdgeCases) {
    // Test with very large vectors
    std::vector<int> large_vector(100000);
    std::iota(large_vector.begin(), large_vector.end(), 0);
    
    auto vector_var = var(large_vector);
    auto sum_calc = calc([](const std::vector<int>& v) {
        return std::accumulate(v.begin(), v.end(), 0LL);
    }, vector_var);
    
    long long expected_sum = static_cast<long long>(99999) * 100000 / 2;
    EXPECT_EQ(sum_calc.get(), expected_sum);
    
    // Test with empty containers
    std::vector<int> empty_vector;
    auto empty_var = var(empty_vector);
    auto empty_calc = calc([](const std::vector<int>& v) {
        return v.size();
    }, empty_var);
    
    EXPECT_EQ(empty_calc.get(), 0);
    
    // Test container modification
    empty_vector.push_back(42);
    empty_var.value(empty_vector);
    EXPECT_EQ(empty_calc.get(), 1);
}

// =============================================================================
// 4. EXCEPTION HANDLING TESTS
// =============================================================================

class StressTestExceptions : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test exception propagation in calculations
TEST_F(StressTestExceptions, ExceptionPropagation) {
    auto source = var(1);
    
    // Create calculation that throws exception
    auto throwing_calc = calc([](int x) -> int {
        if (x == 42) {
            throw std::runtime_error("Test exception");
        }
        return x * 2;
    }, source);
    
    // Should work normally
    EXPECT_EQ(throwing_calc.get(), 2);
    
    // Should throw when condition is met
    source.value(42);
    EXPECT_THROW(throwing_calc.get(), std::runtime_error);
    
    // Should recover when condition is no longer met
    source.value(10);
    EXPECT_EQ(throwing_calc.get(), 20);
}

// Test exception handling in batch operations
TEST_F(StressTestExceptions, ExceptionInBatchOperations) {
    auto source = var(1);
    std::atomic<int> action_count{0};
    
    // Create action that might throw
    auto action_node = action([&action_count](int x) {
        action_count++;
        if (x == 999) {
            throw std::runtime_error("Action exception");
        }
    }, source);
    
    // Normal batch should work
    batchExecute([&]() {
        source.value(10);
    });
    EXPECT_EQ(action_count.load(), 1);
    
    // Batch with exception should still complete other operations
    EXPECT_NO_THROW({
        batchExecute([&]() {
            source.value(999); // This will cause action to throw
        });
    });
    
    // The action should have been called even though it threw
    EXPECT_EQ(action_count.load(), 2);
}

// Test resource cleanup on exceptions
TEST_F(StressTestExceptions, ResourceCleanupOnExceptions) {
    std::atomic<int> destructor_count{0};
    
    class ThrowingResource {
    public:
        ThrowingResource(int value, std::atomic<int>& counter) 
            : m_value(value), m_counter(counter) {}
        
        ~ThrowingResource() {
            m_counter++;
        }
        
        int getValue() const {
            if (m_value == -1) {
                throw std::runtime_error("Resource exception");
            }
            return m_value;
        }
        
    private:
        int m_value;
        std::atomic<int>& m_counter;
    };
    
    {
        auto resource_var = var(ThrowingResource(42, destructor_count));
        auto calc_node = calc([](const ThrowingResource& r) {
            return r.getValue() * 2;
        }, resource_var);
        
        EXPECT_EQ(calc_node.get(), 84);
        
        // This should throw but not leak resources
        resource_var.value(ThrowingResource(-1, destructor_count));
        EXPECT_THROW(calc_node.get(), std::runtime_error);
        
        // Resources should still be properly managed
        resource_var.value(ThrowingResource(100, destructor_count));
        EXPECT_EQ(calc_node.get(), 200);
    }
    
    // All resources should be properly destroyed
    EXPECT_GE(destructor_count.load(), 3); // At least 3 objects were created
}

// Test exception safety in graph modifications
TEST_F(StressTestExceptions, ExceptionSafetyInGraphModifications) {
    auto source = var(1);
    auto calc_node = calc([](int x) { return x * 2; }, source);
    
    // Try to reset with invalid dependencies - should throw but leave node valid
    EXPECT_THROW({
        calc_node.reset([&]() { return calc_node() + 1; }); // Self-dependency
    }, std::runtime_error);
    
    // Node should still be valid and functional
    EXPECT_TRUE(static_cast<bool>(calc_node));
    EXPECT_EQ(calc_node.get(), 2);
    
    // Should be able to update normally
    source.value(10);
    EXPECT_EQ(calc_node.get(), 20);
    
    // Should be able to reset with valid function
    EXPECT_NO_THROW({
        calc_node.reset([](int x) { return x * 3; }, source);
    });
    EXPECT_EQ(calc_node.get(), 30);
}

// Test exception handling with custom strategies
TEST_F(StressTestExceptions, ExceptionHandlingWithCustomStrategies) {
    struct ThrowingStrategy {
        void handleInvalid() {
            throw std::runtime_error("Strategy exception");
        }
    };
    
    {
        auto temp_var = var(42);
        auto throwing_calc = calc<ChangeTrig, ThrowingStrategy>([](int x) { return x; }, temp_var);
        
        EXPECT_EQ(throwing_calc.get(), 42);
        
        // When temp_var goes out of scope, strategy should throw
        // But this should not crash the program
    }
    
    // Program should continue normally after exception
    auto normal_var = var(100);
    EXPECT_EQ(normal_var.get(), 100);
}

// =============================================================================
// Continue with remaining test categories...
// =============================================================================

// Test division by zero and mathematical edge cases
TEST_F(StressTestExceptions, MathematicalExceptions) {
    auto numerator = var(10.0);
    auto denominator = var(2.0);
    
    auto division_calc = calc([](double a, double b) {
        return a / b;
    }, numerator, denominator);
    
    EXPECT_EQ(division_calc.get(), 5.0);
    
    // Division by zero should produce infinity, not throw
    denominator.value(0.0);
    EXPECT_TRUE(std::isinf(division_calc.get()));
    
    // Test with negative zero
    denominator.value(-0.0);
    EXPECT_TRUE(std::isinf(division_calc.get()));
    
    // Test square root of negative number
    auto sqrt_calc = calc([](double x) {
        return std::sqrt(x);
    }, numerator);
    
    numerator.value(-1.0);
    EXPECT_TRUE(std::isnan(sqrt_calc.get()));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}