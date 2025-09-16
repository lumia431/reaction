/*
 * Advanced Bug Finding Tests for Reaction Framework
 * 
 * This file contains tests designed to find additional edge cases and bugs.
 */

#include "reaction/reaction.h"
#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <random>
#include <atomic>

using namespace reaction;

void test_null_pointer_dereference() {
    std::cout << "=== Testing Null Pointer Dereference ===" << std::endl;
    
    try {
        // Test with moved-from object
        auto original = var(42);
        auto moved = std::move(original);
        
        std::cout << "Original valid: " << static_cast<bool>(original) << std::endl;
        std::cout << "Moved valid: " << static_cast<bool>(moved) << std::endl;
        
        // This should throw, not crash
        try {
            original.get();
            std::cout << "❌ original.get() should have thrown!" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "✅ original.get() correctly threw: " << e.what() << std::endl;
        }
        
        // Test calculation with moved-from dependency
        try {
            auto calc_with_moved = calc([](int x) { return x * 2; }, original);
            std::cout << "❌ calc with moved dependency should have failed!" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "✅ calc with moved dependency correctly threw: " << e.what() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Unexpected exception in null pointer test: " << e.what() << std::endl;
    }
}

void test_rapid_dependency_changes() {
    std::cout << "\n=== Testing Rapid Dependency Changes ===" << std::endl;
    
    try {
        auto a = var(1);
        auto b = var(2);
        auto c = var(3);
        
        auto dynamic_calc = calc([](int x) { return x * 10; }, a);
        
        // Rapidly change dependencies
        for (int i = 0; i < 100; ++i) {
            if (i % 3 == 0) {
                dynamic_calc.reset([](int x) { return x * 10; }, a);
            } else if (i % 3 == 1) {
                dynamic_calc.reset([](int x) { return x * 20; }, b);
            } else {
                dynamic_calc.reset([](int x) { return x * 30; }, c);
            }
            
            // Update source values
            a.value(a.get() + 1);
            b.value(b.get() + 1);
            c.value(c.get() + 1);
            
            // Get result
            int result = dynamic_calc.get();
            (void)result; // Suppress unused warning
        }
        
        std::cout << "✅ Rapid dependency changes completed without crash" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception in rapid dependency changes: " << e.what() << std::endl;
    }
}

void test_recursive_batch_operations() {
    std::cout << "\n=== Testing Recursive Batch Operations ===" << std::endl;
    
    try {
        auto source = var(1);
        
        std::atomic<int> batch_depth{0};
        std::atomic<int> max_depth{0};
        
        auto recursive_calc = calc([&](int x) -> int {
            batch_depth++;
            int current_depth = batch_depth.load();
            max_depth = std::max(max_depth.load(), current_depth);
            
            if (current_depth < 5) {
                // Trigger nested batch
                batchExecute([&]() {
                    source.value(source.get() + 1);
                });
            }
            
            batch_depth--;
            return x * 2;
        }, source);
        
        // This might cause infinite recursion or other issues
        batchExecute([&]() {
            source.value(10);
        });
        
        std::cout << "✅ Recursive batch operations completed" << std::endl;
        std::cout << "Max batch depth reached: " << max_depth.load() << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception in recursive batch operations: " << e.what() << std::endl;
    }
}

void test_memory_corruption() {
    std::cout << "\n=== Testing Memory Corruption Scenarios ===" << std::endl;
    
    try {
        // Test with dangling references in lambdas
        std::vector<Calc<int>> dangling_calcs;
        
        {
            int local_value = 42;
            auto source = var(1);
            
            // This lambda captures local_value by reference
            // When local_value goes out of scope, this becomes a dangling reference
            dangling_calcs.push_back(calc([&local_value](int x) {
                return x + local_value; // Potential dangling reference
            }, source));
            
            std::cout << "Initial result: " << dangling_calcs[0].get() << std::endl;
            // local_value goes out of scope here
        }
        
        // This might access dangling memory
        try {
            int result = dangling_calcs[0].get();
            std::cout << "⚠️  Dangling reference result: " << result << " (might be garbage)" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "✅ Dangling reference correctly detected: " << e.what() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception in memory corruption test: " << e.what() << std::endl;
    }
}

void test_concurrent_graph_modification() {
    std::cout << "\n=== Testing Concurrent Graph Modification ===" << std::endl;
    
    try {
        std::vector<Var<int>> sources(10);
        for (int i = 0; i < 10; ++i) {
            sources[i] = var(i);
        }
        
        std::vector<Calc<int>> calculations(10);
        for (int i = 0; i < 10; ++i) {
            calculations[i] = calc([](int x) { return x * 2; }, sources[i]);
        }
        
        std::atomic<int> error_count{0};
        std::vector<std::future<void>> futures;
        
        // Thread 1: Modify dependencies
        futures.push_back(std::async(std::launch::async, [&]() {
            try {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, 9);
                
                for (int i = 0; i < 1000; ++i) {
                    int calc_idx = dis(gen);
                    int source_idx = dis(gen);
                    
                    calculations[calc_idx].reset([](int x) { return x * 3; }, sources[source_idx]);
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            } catch (...) {
                error_count++;
            }
        }));
        
        // Thread 2: Update values
        futures.push_back(std::async(std::launch::async, [&]() {
            try {
                for (int i = 0; i < 1000; ++i) {
                    for (int j = 0; j < 10; ++j) {
                        sources[j].value(sources[j].get() + 1);
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            } catch (...) {
                error_count++;
            }
        }));
        
        // Thread 3: Read values
        futures.push_back(std::async(std::launch::async, [&]() {
            try {
                for (int i = 0; i < 1000; ++i) {
                    for (int j = 0; j < 10; ++j) {
                        int result = calculations[j].get();
                        (void)result;
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            } catch (...) {
                error_count++;
            }
        }));
        
        // Wait for all threads
        for (auto& future : futures) {
            future.wait();
        }
        
        if (error_count.load() == 0) {
            std::cout << "✅ Concurrent graph modification completed without errors" << std::endl;
        } else {
            std::cout << "❌ " << error_count.load() << " errors occurred during concurrent modification" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception in concurrent graph modification: " << e.what() << std::endl;
    }
}

void test_extreme_values() {
    std::cout << "\n=== Testing Extreme Values ===" << std::endl;
    
    try {
        // Test with very large numbers
        auto large_var = var(std::numeric_limits<int>::max());
        auto large_calc = calc([](int x) {
            return x + 1; // This will overflow
        }, large_var);
        
        std::cout << "Max int: " << std::numeric_limits<int>::max() << std::endl;
        std::cout << "Max int + 1: " << large_calc.get() << " (overflow expected)" << std::endl;
        
        // Test with very small numbers
        auto small_var = var(std::numeric_limits<int>::min());
        auto small_calc = calc([](int x) {
            return x - 1; // This will underflow
        }, small_var);
        
        std::cout << "Min int: " << std::numeric_limits<int>::min() << std::endl;
        std::cout << "Min int - 1: " << small_calc.get() << " (underflow expected)" << std::endl;
        
        // Test with NaN and infinity
        auto nan_var = var(std::numeric_limits<double>::quiet_NaN());
        auto inf_var = var(std::numeric_limits<double>::infinity());
        
        auto nan_calc = calc([](double x) { return x * 2; }, nan_var);
        auto inf_calc = calc([](double x) { return x + 1; }, inf_var);
        
        std::cout << "NaN * 2: " << nan_calc.get() << std::endl;
        std::cout << "Inf + 1: " << inf_calc.get() << std::endl;
        
        std::cout << "✅ Extreme values test completed" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception in extreme values test: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Advanced Bug Finding Tests for Reaction Framework" << std::endl;
    std::cout << "=================================================" << std::endl;
    
    test_null_pointer_dereference();
    test_rapid_dependency_changes();
    test_recursive_batch_operations();
    test_memory_corruption();
    test_concurrent_graph_modification();
    test_extreme_values();
    
    std::cout << "\n=================================================" << std::endl;
    std::cout << "Advanced bug finding tests completed!" << std::endl;
    std::cout << "Check output above for any issues found." << std::endl;
    
    return 0;
}