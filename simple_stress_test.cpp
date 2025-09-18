/*
 * Simple Stress Test for Reaction Framework (No External Dependencies)
 * 
 * This test doesn't require gtest and can be compiled with just g++.
 * It focuses on finding basic bugs and issues in the framework.
 */

#include "reaction/reaction.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <cassert>
#include <exception>
#include <atomic>
#include <future>

using namespace reaction;
using namespace std::chrono;

// Simple assertion macro
#define SIMPLE_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "ASSERTION FAILED: " << message << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            test_failures++; \
        } else { \
            test_successes++; \
        } \
    } while(0)

#define SIMPLE_EXPECT_THROW(expression, exception_type, message) \
    do { \
        bool threw_expected = false; \
        try { \
            expression; \
        } catch (const exception_type&) { \
            threw_expected = true; \
        } catch (...) { \
            std::cerr << "UNEXPECTED EXCEPTION: " << message << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            test_failures++; \
            break; \
        } \
        if (threw_expected) { \
            test_successes++; \
        } else { \
            std::cerr << "EXPECTED EXCEPTION NOT THROWN: " << message << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            test_failures++; \
        } \
    } while(0)

// Global test counters
std::atomic<int> test_successes{0};
std::atomic<int> test_failures{0};

void print_test_header(const std::string& test_name) {
    std::cout << "\n=== " << test_name << " ===" << std::endl;
}

void print_test_result(const std::string& test_name, bool passed) {
    if (passed) {
        std::cout << "✅ " << test_name << " PASSED" << std::endl;
    } else {
        std::cout << "❌ " << test_name << " FAILED" << std::endl;
    }
}

// Test 1: Basic functionality
void test_basic_functionality() {
    print_test_header("Basic Functionality Test");
    
    try {
        auto a = var(1);
        auto b = var(3.14);
        
        SIMPLE_ASSERT(a.get() == 1, "Variable a should be 1");
        SIMPLE_ASSERT(b.get() == 3.14, "Variable b should be 3.14");
        
        auto ds = calc([](int aa, double bb) { return aa + bb; }, a, b);
        SIMPLE_ASSERT(std::abs(ds.get() - 4.14) < 0.001, "Calculation should be 4.14");
        
        a.value(2);
        SIMPLE_ASSERT(std::abs(ds.get() - 5.14) < 0.001, "Calculation should update to 5.14");
        
        print_test_result("Basic Functionality", true);
    } catch (const std::exception& e) {
        std::cerr << "Exception in basic functionality test: " << e.what() << std::endl;
        print_test_result("Basic Functionality", false);
    }
}

// Test 2: Memory stress test
void test_memory_stress() {
    print_test_header("Memory Stress Test");
    
    try {
        constexpr int NODE_COUNT = 10000;
        std::vector<Var<int>> nodes;
        nodes.reserve(NODE_COUNT);
        
        // Create many nodes
        for (int i = 0; i < NODE_COUNT; ++i) {
            nodes.push_back(var(i));
        }
        
        // Verify all nodes
        bool all_correct = true;
        for (int i = 0; i < NODE_COUNT; ++i) {
            if (nodes[i].get() != i) {
                all_correct = false;
                break;
            }
        }
        
        SIMPLE_ASSERT(all_correct, "All nodes should have correct values");
        
        // Update random nodes
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, NODE_COUNT - 1);
        
        for (int i = 0; i < 1000; ++i) {
            int idx = dis(gen);
            nodes[idx].value(idx * 2);
            SIMPLE_ASSERT(nodes[idx].get() == idx * 2, "Updated node should have correct value");
        }
        
        print_test_result("Memory Stress", true);
    } catch (const std::exception& e) {
        std::cerr << "Exception in memory stress test: " << e.what() << std::endl;
        print_test_result("Memory Stress", false);
    }
}

// Test 3: Circular dependency detection
void test_circular_dependency() {
    print_test_header("Circular Dependency Test");
    
    try {
        auto a = var(1);
        auto dsA = calc([](int aa) { return aa; }, a);
        
        // This should throw due to self-dependency
        SIMPLE_EXPECT_THROW(
            dsA.reset([&]() { return a() + dsA(); }),
            std::runtime_error,
            "Self-dependency should throw"
        );
        
        // Test circular dependency between multiple nodes
        auto b = var(2);
        auto c = var(3);
        auto dsB = calc([](int bb) { return bb; }, b);
        auto dsC = calc([](int cc) { return cc; }, c);
        
        dsA.reset([&]() { return b() + dsB(); });
        dsB.reset([&]() { return c() * dsC(); });
        
        // This should create a cycle and throw
        SIMPLE_EXPECT_THROW(
            dsC.reset([&]() { return a() - dsA(); }),
            std::runtime_error,
            "Circular dependency should throw"
        );
        
        print_test_result("Circular Dependency", true);
    } catch (const std::exception& e) {
        std::cerr << "Exception in circular dependency test: " << e.what() << std::endl;
        print_test_result("Circular Dependency", false);
    }
}

// Test 4: Batch operations
void test_batch_operations() {
    print_test_header("Batch Operations Test");
    
    try {
        auto a = var(1);
        auto b = var(2);
        
        int trigger_count = 0;
        auto ds = calc([&trigger_count](int aa, int bb) { 
            trigger_count++; 
            return aa + bb; 
        }, a, b);
        
        SIMPLE_ASSERT(ds.get() == 3, "Initial calculation should be 3");
        SIMPLE_ASSERT(trigger_count == 1, "Should have triggered once initially");
        
        trigger_count = 0;
        
        // Without batch - should trigger twice
        a.value(10);
        b.value(20);
        SIMPLE_ASSERT(trigger_count == 2, "Should trigger twice without batch");
        SIMPLE_ASSERT(ds.get() == 30, "Result should be 30");
        
        trigger_count = 0;
        
        // With batch - should trigger once
        batchExecute([&]() {
            a.value(100);
            b.value(200);
        });
        
        SIMPLE_ASSERT(trigger_count == 1, "Should trigger once with batch");
        SIMPLE_ASSERT(ds.get() == 300, "Result should be 300");
        
        print_test_result("Batch Operations", true);
    } catch (const std::exception& e) {
        std::cerr << "Exception in batch operations test: " << e.what() << std::endl;
        print_test_result("Batch Operations", false);
    }
}

// Test 5: Deep dependency chain
void test_deep_dependency_chain() {
    print_test_header("Deep Dependency Chain Test");
    
    try {
        constexpr int DEPTH = 1000;
        
        auto root = var(1);
        std::vector<Calc<int>> chain;
        chain.reserve(DEPTH);
        
        chain.push_back(calc([](int x) { return x + 1; }, root));
        for (int i = 1; i < DEPTH; ++i) {
            chain.push_back(calc([](int x) { return x + 1; }, chain[i-1]));
        }
        
        SIMPLE_ASSERT(chain[DEPTH-1].get() == DEPTH + 1, "Deep chain should have correct final value");
        
        root.value(100);
        SIMPLE_ASSERT(chain[DEPTH-1].get() == 100 + DEPTH, "Deep chain should update correctly");
        
        print_test_result("Deep Dependency Chain", true);
    } catch (const std::exception& e) {
        std::cerr << "Exception in deep dependency chain test: " << e.what() << std::endl;
        print_test_result("Deep Dependency Chain", false);
    }
}

// Test 6: Concurrent operations
void test_concurrent_operations() {
    print_test_header("Concurrent Operations Test");
    
    try {
        constexpr int THREAD_COUNT = 4;
        constexpr int OPERATIONS_PER_THREAD = 1000;
        
        std::vector<Var<int>> vars(THREAD_COUNT);
        for (int i = 0; i < THREAD_COUNT; ++i) {
            vars[i] = var(i);
        }
        
        std::vector<std::future<void>> futures;
        std::atomic<int> error_count{0};
        
        for (int t = 0; t < THREAD_COUNT; ++t) {
            futures.push_back(std::async(std::launch::async, [t, &vars, &error_count]() {
                try {
                    for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                        vars[t].value(vars[t].get() + 1);
                    }
                } catch (...) {
                    error_count++;
                }
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
        
        SIMPLE_ASSERT(error_count.load() == 0, "No errors should occur in concurrent operations");
        
        // Verify final values
        for (int i = 0; i < THREAD_COUNT; ++i) {
            int expected = i + OPERATIONS_PER_THREAD;
            SIMPLE_ASSERT(vars[i].get() == expected, "Concurrent updates should be correct");
        }
        
        print_test_result("Concurrent Operations", true);
    } catch (const std::exception& e) {
        std::cerr << "Exception in concurrent operations test: " << e.what() << std::endl;
        print_test_result("Concurrent Operations", false);
    }
}

// Test 7: Exception handling
void test_exception_handling() {
    print_test_header("Exception Handling Test");
    
    try {
        auto source = var(1);
        
        auto throwing_calc = calc([](int x) -> int {
            if (x == 42) {
                throw std::runtime_error("Test exception");
            }
            return x * 2;
        }, source);
        
        SIMPLE_ASSERT(throwing_calc.get() == 2, "Normal operation should work");
        
        source.value(42);
        SIMPLE_EXPECT_THROW(throwing_calc.get(), std::runtime_error, "Should throw when condition is met");
        
        source.value(10);
        SIMPLE_ASSERT(throwing_calc.get() == 20, "Should recover after exception");
        
        print_test_result("Exception Handling", true);
    } catch (const std::exception& e) {
        std::cerr << "Exception in exception handling test: " << e.what() << std::endl;
        print_test_result("Exception Handling", false);
    }
}

// Test 8: Field-based objects
void test_field_objects() {
    print_test_header("Field-Based Objects Test");
    
    try {
        class Person : public FieldBase {
        public:
            Person(std::string name, int age) : 
                m_name(field(name)), 
                m_age(field(age)) {}
            
            std::string getName() const { return m_name.get(); }
            void setName(const std::string &name) { m_name.value(name); }
            
            int getAge() const { return m_age.get(); }
            void setAge(int age) { m_age.value(age); }
            
        private:
            Var<std::string> m_name;
            Var<int> m_age;
        };
        
        Person person{"Alice", 25};
        auto p = var(person);
        
        auto name_calc = calc([](const Person& person) {
            return person.getName();
        }, p);
        
        SIMPLE_ASSERT(name_calc.get() == "Alice", "Field should have correct initial value");
        
        p->setName("Bob");
        SIMPLE_ASSERT(name_calc.get() == "Bob", "Field update should propagate");
        
        p->setAge(30);
        SIMPLE_ASSERT(p->getAge() == 30, "Age field should update");
        
        print_test_result("Field-Based Objects", true);
    } catch (const std::exception& e) {
        std::cerr << "Exception in field-based objects test: " << e.what() << std::endl;
        print_test_result("Field-Based Objects", false);
    }
}

// Test 9: Expression templates
void test_expression_templates() {
    print_test_header("Expression Templates Test");
    
    try {
        auto a = var(1);
        auto b = var(2);
        auto c = var(3.14);
        
        auto expr_result = expr(c + a / b - a * 2);
        
        // Expected: 3.14 + 1/2 - 1*2 = 3.14 + 0.5 - 2 = 1.64
        SIMPLE_ASSERT(std::abs(expr_result.get() - 1.64) < 0.001, "Expression should evaluate correctly");
        
        a.value(4);
        // Expected: 3.14 + 4/2 - 4*2 = 3.14 + 2 - 8 = -2.86
        SIMPLE_ASSERT(std::abs(expr_result.get() - (-2.86)) < 0.001, "Expression should update correctly");
        
        print_test_result("Expression Templates", true);
    } catch (const std::exception& e) {
        std::cerr << "Exception in expression templates test: " << e.what() << std::endl;
        print_test_result("Expression Templates", false);
    }
}

// Test 10: Performance benchmark
void test_performance_benchmark() {
    print_test_header("Performance Benchmark Test");
    
    try {
        constexpr int PERFORMANCE_NODES = 10000;
        constexpr int PERFORMANCE_UPDATES = 1000;
        
        std::vector<Var<int>> perf_vars;
        std::vector<Calc<int>> perf_calcs;
        
        perf_vars.reserve(PERFORMANCE_NODES);
        perf_calcs.reserve(PERFORMANCE_NODES);
        
        auto start_time = high_resolution_clock::now();
        
        // Create nodes
        for (int i = 0; i < PERFORMANCE_NODES; ++i) {
            perf_vars.push_back(var(i));
            perf_calcs.push_back(calc([](int x) { return x * 2; }, perf_vars[i]));
        }
        
        auto creation_time = high_resolution_clock::now();
        auto creation_duration = duration_cast<milliseconds>(creation_time - start_time);
        
        // Update nodes
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, PERFORMANCE_NODES - 1);
        
        for (int i = 0; i < PERFORMANCE_UPDATES; ++i) {
            int idx = dis(gen);
            perf_vars[idx].value(perf_vars[idx].get() + 1);
        }
        
        auto update_time = high_resolution_clock::now();
        auto update_duration = duration_cast<milliseconds>(update_time - creation_time);
        
        std::cout << "Performance Results:" << std::endl;
        std::cout << "  Node creation: " << creation_duration.count() << "ms for " << PERFORMANCE_NODES << " nodes" << std::endl;
        std::cout << "  Updates: " << update_duration.count() << "ms for " << PERFORMANCE_UPDATES << " updates" << std::endl;
        
        // Verify some calculations are correct
        bool calculations_correct = true;
        for (int i = 0; i < std::min(100, PERFORMANCE_NODES); ++i) {
            if (perf_calcs[i].get() != perf_vars[i].get() * 2) {
                calculations_correct = false;
                break;
            }
        }
        
        SIMPLE_ASSERT(calculations_correct, "Performance test calculations should be correct");
        
        print_test_result("Performance Benchmark", true);
    } catch (const std::exception& e) {
        std::cerr << "Exception in performance benchmark test: " << e.what() << std::endl;
        print_test_result("Performance Benchmark", false);
    }
}

int main() {
    std::cout << "=========================================" << std::endl;
    std::cout << "Simple Stress Test for Reaction Framework" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    // Run all tests
    test_basic_functionality();
    test_memory_stress();
    test_circular_dependency();
    test_batch_operations();
    test_deep_dependency_chain();
    test_concurrent_operations();
    test_exception_handling();
    test_field_objects();
    test_expression_templates();
    test_performance_benchmark();
    
    // Print final results
    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "✅ Successes: " << test_successes.load() << std::endl;
    std::cout << "❌ Failures: " << test_failures.load() << std::endl;
    
    if (test_failures.load() == 0) {
        std::cout << "\n🎉 ALL TESTS PASSED! Framework appears to be working correctly." << std::endl;
        std::cout << "\nPotential areas for further investigation:" << std::endl;
        std::cout << "1. More extensive concurrency testing" << std::endl;
        std::cout << "2. Memory leak detection with valgrind" << std::endl;
        std::cout << "3. Performance regression testing" << std::endl;
        std::cout << "4. Platform-specific testing (Windows, macOS)" << std::endl;
        std::cout << "5. Compiler-specific testing (Clang, MSVC)" << std::endl;
        return 0;
    } else {
        std::cout << "\n⚠️  SOME TESTS FAILED! Please investigate the issues above." << std::endl;
        std::cout << "\nCommon causes of failures:" << std::endl;
        std::cout << "1. Compiler doesn't fully support C++20" << std::endl;
        std::cout << "2. Framework headers have issues" << std::endl;
        std::cout << "3. Threading/concurrency bugs" << std::endl;
        std::cout << "4. Memory management issues" << std::endl;
        return 1;
    }
}