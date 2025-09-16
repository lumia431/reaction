/*
 * Performance and Scalability Stress Tests for Reaction Framework
 * 
 * These tests focus on finding performance bottlenecks, memory leaks,
 * and scalability issues under extreme conditions.
 */

#include "reaction/reaction.h"
#include "gtest/gtest.h"
#include <chrono>
#include <vector>
#include <random>
#include <algorithm>
#include <thread>
#include <future>
#include <numeric>

using namespace reaction;
using namespace std::chrono;

class PerformanceStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        start_time = high_resolution_clock::now();
    }
    
    void TearDown() override {
        auto end_time = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end_time - start_time);
        std::cout << "Test completed in " << duration.count() << "ms" << std::endl;
    }
    
    high_resolution_clock::time_point start_time;
};

// Test massive dependency graph creation and updates
TEST_F(PerformanceStressTest, MassiveDependencyGraph) {
    constexpr int NODES = 50000;
    constexpr int DEPENDENCIES_PER_NODE = 5;
    
    std::vector<Var<int>> sources;
    std::vector<Calc<int>> calculations;
    
    // Create source nodes
    sources.reserve(NODES);
    for (int i = 0; i < NODES; ++i) {
        sources.push_back(var(i));
    }
    
    // Create calculations with random dependencies
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, NODES - 1);
    
    calculations.reserve(NODES);
    for (int i = 0; i < NODES; ++i) {
        // Create calculation that depends on multiple random sources
        std::vector<int> deps;
        for (int j = 0; j < DEPENDENCIES_PER_NODE; ++j) {
            deps.push_back(dis(gen));
        }
        
        calculations.push_back(calc([deps, &sources]() {
            int sum = 0;
            for (int dep : deps) {
                sum += sources[dep]();
            }
            return sum;
        }));
    }
    
    // Measure update performance
    auto update_start = high_resolution_clock::now();
    
    // Update random sources
    for (int i = 0; i < 1000; ++i) {
        int idx = dis(gen);
        sources[idx].value(sources[idx].get() + 1);
    }
    
    auto update_end = high_resolution_clock::now();
    auto update_duration = duration_cast<microseconds>(update_end - update_start);
    
    std::cout << "Update performance: " << update_duration.count() << " microseconds for 1000 updates" << std::endl;
    
    // Verify all calculations are still valid
    for (const auto& calc : calculations) {
        EXPECT_TRUE(static_cast<bool>(calc));
        EXPECT_NO_THROW(calc.get());
    }
}

// Test batch performance with many nodes
TEST_F(PerformanceStressTest, BatchPerformanceStress) {
    constexpr int BATCH_SIZE = 10000;
    std::vector<Var<int>> vars;
    std::vector<Calc<int>> observers;
    
    // Create variables and observers
    vars.reserve(BATCH_SIZE);
    observers.reserve(BATCH_SIZE);
    
    for (int i = 0; i < BATCH_SIZE; ++i) {
        vars.push_back(var(i));
        observers.push_back(calc([](int x) { return x * 2; }, vars[i]));
    }
    
    // Measure batch performance
    auto batch_start = high_resolution_clock::now();
    
    batchExecute([&]() {
        for (int i = 0; i < BATCH_SIZE; ++i) {
            vars[i].value(vars[i].get() + 1);
        }
    });
    
    auto batch_end = high_resolution_clock::now();
    auto batch_duration = duration_cast<microseconds>(batch_end - batch_start);
    
    std::cout << "Batch performance: " << batch_duration.count() << " microseconds for " << BATCH_SIZE << " updates" << std::endl;
    
    // Verify all observers updated correctly
    for (int i = 0; i < BATCH_SIZE; ++i) {
        EXPECT_EQ(observers[i].get(), (i + 1) * 2);
    }
    
    // Compare with individual updates
    auto individual_start = high_resolution_clock::now();
    
    for (int i = 0; i < BATCH_SIZE; ++i) {
        vars[i].value(vars[i].get() + 1);
    }
    
    auto individual_end = high_resolution_clock::now();
    auto individual_duration = duration_cast<microseconds>(individual_end - individual_start);
    
    std::cout << "Individual update performance: " << individual_duration.count() << " microseconds for " << BATCH_SIZE << " updates" << std::endl;
    
    // Batch should be significantly faster
    EXPECT_LT(batch_duration.count(), individual_duration.count());
}

// Test memory usage under stress
TEST_F(PerformanceStressTest, MemoryUsageStress) {
    constexpr int ITERATIONS = 1000;
    constexpr int NODES_PER_ITERATION = 1000;
    
    for (int iter = 0; iter < ITERATIONS; ++iter) {
        std::vector<Var<std::string>> temp_vars;
        std::vector<Calc<std::string>> temp_calcs;
        
        temp_vars.reserve(NODES_PER_ITERATION);
        temp_calcs.reserve(NODES_PER_ITERATION);
        
        // Create temporary nodes
        for (int i = 0; i < NODES_PER_ITERATION; ++i) {
            temp_vars.push_back(var(std::string("temp_") + std::to_string(i)));
            temp_calcs.push_back(calc([](const std::string& s) {
                return s + "_processed";
            }, temp_vars[i]));
        }
        
        // Use the nodes
        for (int i = 0; i < NODES_PER_ITERATION; ++i) {
            temp_vars[i].value(std::string("updated_") + std::to_string(i));
            std::string result = temp_calcs[i].get();
            EXPECT_EQ(result, std::string("updated_") + std::to_string(i) + "_processed");
        }
        
        // Nodes will be destroyed at end of iteration
        // This tests memory cleanup
        
        if (iter % 100 == 0) {
            std::cout << "Completed iteration " << iter << std::endl;
        }
    }
}

// Test deep recursion performance
TEST_F(PerformanceStressTest, DeepRecursionPerformance) {
    constexpr int DEPTH = 10000;
    
    auto root = var(1);
    std::vector<Calc<int>> chain;
    chain.reserve(DEPTH);
    
    // Create deep chain
    chain.push_back(calc([](int x) { return x + 1; }, root));
    for (int i = 1; i < DEPTH; ++i) {
        chain.push_back(calc([](int x) { return x + 1; }, chain[i-1]));
    }
    
    // Measure propagation time
    auto propagation_start = high_resolution_clock::now();
    
    root.value(100);
    int final_value = chain[DEPTH-1].get();
    
    auto propagation_end = high_resolution_clock::now();
    auto propagation_duration = duration_cast<microseconds>(propagation_end - propagation_start);
    
    std::cout << "Deep propagation performance: " << propagation_duration.count() << " microseconds for depth " << DEPTH << std::endl;
    
    EXPECT_EQ(final_value, 100 + DEPTH);
    
    // Test batch propagation
    auto batch_start = high_resolution_clock::now();
    
    batchExecute([&]() {
        root.value(200);
    });
    
    final_value = chain[DEPTH-1].get();
    
    auto batch_propagation_end = high_resolution_clock::now();
    auto batch_propagation_duration = duration_cast<microseconds>(batch_propagation_end - batch_start);
    
    std::cout << "Deep batch propagation performance: " << batch_propagation_duration.count() << " microseconds for depth " << DEPTH << std::endl;
    
    EXPECT_EQ(final_value, 200 + DEPTH);
}

// Test wide dependency tree performance
TEST_F(PerformanceStressTest, WideDependencyTreePerformance) {
    constexpr int WIDTH = 10000;
    
    std::vector<Var<double>> leaves;
    leaves.reserve(WIDTH);
    
    // Create leaf nodes
    for (int i = 0; i < WIDTH; ++i) {
        leaves.push_back(var(static_cast<double>(i)));
    }
    
    // Create aggregator that depends on all leaves
    auto aggregator = calc([&leaves]() {
        double sum = 0.0;
        for (const auto& leaf : leaves) {
            sum += leaf();
        }
        return sum;
    });
    
    double expected_sum = static_cast<double>(WIDTH - 1) * WIDTH / 2;
    EXPECT_NEAR(aggregator.get(), expected_sum, 0.001);
    
    // Measure update performance
    auto update_start = high_resolution_clock::now();
    
    // Update all leaves in batch
    batchExecute([&]() {
        for (int i = 0; i < WIDTH; ++i) {
            leaves[i].value(static_cast<double>(i * 2));
        }
    });
    
    auto update_end = high_resolution_clock::now();
    auto update_duration = duration_cast<microseconds>(update_end - update_start);
    
    std::cout << "Wide tree update performance: " << update_duration.count() << " microseconds for " << WIDTH << " leaves" << std::endl;
    
    expected_sum = static_cast<double>(WIDTH - 1) * WIDTH;
    EXPECT_NEAR(aggregator.get(), expected_sum, 0.001);
}

// Test concurrent performance
TEST_F(PerformanceStressTest, ConcurrentPerformanceStress) {
    constexpr int THREAD_COUNT = std::thread::hardware_concurrency();
    constexpr int OPERATIONS_PER_THREAD = 10000;
    
    std::vector<Var<int>> shared_vars(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; ++i) {
        shared_vars[i] = var(i);
    }
    
    // Create observers
    std::vector<Calc<int>> observers;
    for (int i = 0; i < THREAD_COUNT; ++i) {
        observers.push_back(calc([](int x) { return x * x; }, shared_vars[i]));
    }
    
    auto concurrent_start = high_resolution_clock::now();
    
    std::vector<std::future<void>> futures;
    
    // Launch concurrent operations
    for (int t = 0; t < THREAD_COUNT; ++t) {
        futures.push_back(std::async(std::launch::async, [t, &shared_vars]() {
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                shared_vars[t].value(shared_vars[t].get() + 1);
            }
        }));
    }
    
    // Wait for completion
    for (auto& future : futures) {
        future.wait();
    }
    
    auto concurrent_end = high_resolution_clock::now();
    auto concurrent_duration = duration_cast<milliseconds>(concurrent_end - concurrent_start);
    
    std::cout << "Concurrent performance: " << concurrent_duration.count() << "ms for " 
              << (THREAD_COUNT * OPERATIONS_PER_THREAD) << " operations across " 
              << THREAD_COUNT << " threads" << std::endl;
    
    // Verify final values
    for (int i = 0; i < THREAD_COUNT; ++i) {
        int expected = i + OPERATIONS_PER_THREAD;
        EXPECT_EQ(shared_vars[i].get(), expected);
        EXPECT_EQ(observers[i].get(), expected * expected);
    }
}

// Test expression complexity performance
TEST_F(PerformanceStressTest, ExpressionComplexityPerformance) {
    constexpr int EXPR_COUNT = 1000;
    
    std::vector<Var<double>> vars;
    vars.reserve(EXPR_COUNT);
    
    for (int i = 0; i < EXPR_COUNT; ++i) {
        vars.push_back(var(static_cast<double>(i + 1)));
    }
    
    // Create complex nested expression
    auto complex_expr = expr(vars[0] + vars[1]);
    for (int i = 2; i < EXPR_COUNT; ++i) {
        if (i % 2 == 0) {
            complex_expr = expr(complex_expr + vars[i]);
        } else {
            complex_expr = expr(complex_expr * vars[i] / vars[i]); // Multiply then divide to keep values reasonable
        }
    }
    
    auto evaluation_start = high_resolution_clock::now();
    
    double result = complex_expr.get();
    
    auto evaluation_end = high_resolution_clock::now();
    auto evaluation_duration = duration_cast<microseconds>(evaluation_end - evaluation_start);
    
    std::cout << "Complex expression evaluation: " << evaluation_duration.count() 
              << " microseconds for " << EXPR_COUNT << " variables" << std::endl;
    
    EXPECT_GT(result, 0.0);
    
    // Test update propagation through complex expression
    auto update_start = high_resolution_clock::now();
    
    vars[0].value(100.0);
    double updated_result = complex_expr.get();
    
    auto update_propagation_end = high_resolution_clock::now();
    auto update_propagation_duration = duration_cast<microseconds>(update_propagation_end - update_start);
    
    std::cout << "Complex expression update propagation: " << update_propagation_duration.count() 
              << " microseconds" << std::endl;
    
    EXPECT_NE(result, updated_result);
}

// Test field-based object performance
TEST_F(PerformanceStressTest, FieldObjectPerformance) {
    constexpr int OBJECT_COUNT = 10000;
    
    class PerformanceObject : public FieldBase {
    public:
        PerformanceObject(int id) : 
            m_id(field(id)),
            m_value(field(id * 10)),
            m_name(field(std::string("object_") + std::to_string(id))) {}
        
        void updateValue(int new_value) { m_value.value(new_value); }
        void updateName(const std::string& new_name) { m_name.value(new_name); }
        
        int getId() const { return m_id.get(); }
        int getValue() const { return m_value.get(); }
        std::string getName() const { return m_name.get(); }
        
    private:
        Var<int> m_id;
        Var<int> m_value;
        Var<std::string> m_name;
    };
    
    std::vector<Var<PerformanceObject>> objects;
    std::vector<Calc<std::string>> observers;
    
    objects.reserve(OBJECT_COUNT);
    observers.reserve(OBJECT_COUNT);
    
    // Create objects and observers
    for (int i = 0; i < OBJECT_COUNT; ++i) {
        objects.push_back(var(PerformanceObject(i)));
        observers.push_back(calc([](const PerformanceObject& obj) {
            return obj.getName() + "_" + std::to_string(obj.getValue());
        }, objects[i]));
    }
    
    // Measure field update performance
    auto field_update_start = high_resolution_clock::now();
    
    for (int i = 0; i < OBJECT_COUNT; ++i) {
        objects[i]->updateValue(i * 100);
    }
    
    auto field_update_end = high_resolution_clock::now();
    auto field_update_duration = duration_cast<milliseconds>(field_update_end - field_update_start);
    
    std::cout << "Field update performance: " << field_update_duration.count() 
              << "ms for " << OBJECT_COUNT << " objects" << std::endl;
    
    // Verify updates
    for (int i = 0; i < OBJECT_COUNT; ++i) {
        std::string expected = std::string("object_") + std::to_string(i) + "_" + std::to_string(i * 100);
        EXPECT_EQ(observers[i].get(), expected);
    }
    
    // Test batch field updates
    auto batch_field_start = high_resolution_clock::now();
    
    batchExecute([&]() {
        for (int i = 0; i < OBJECT_COUNT; ++i) {
            objects[i]->updateValue(i * 200);
            objects[i]->updateName(std::string("updated_object_") + std::to_string(i));
        }
    });
    
    auto batch_field_end = high_resolution_clock::now();
    auto batch_field_duration = duration_cast<milliseconds>(batch_field_end - batch_field_start);
    
    std::cout << "Batch field update performance: " << batch_field_duration.count() 
              << "ms for " << (OBJECT_COUNT * 2) << " field updates" << std::endl;
    
    // Verify batch updates
    for (int i = 0; i < OBJECT_COUNT; ++i) {
        std::string expected = std::string("updated_object_") + std::to_string(i) + "_" + std::to_string(i * 200);
        EXPECT_EQ(observers[i].get(), expected);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}