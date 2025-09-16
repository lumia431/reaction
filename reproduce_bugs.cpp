/*
 * Bug Reproduction Script for Reaction Framework
 * 
 * This script demonstrates the critical bugs found during stress testing.
 * Use this to verify bug fixes.
 */

#include "reaction/reaction.h"
#include <iostream>
#include <thread>
#include <future>
#include <vector>
#include <chrono>

using namespace reaction;

void reproduce_exception_crash() {
    std::cout << "=== BUG 1: Exception Handling Crash ===" << std::endl;
    std::cout << "Expected: Program should handle exception gracefully" << std::endl;
    std::cout << "Actual: Program crashes with 'terminate called after throwing'" << std::endl;
    std::cout << "\nReproducing..." << std::endl;
    
    try {
        auto source = var(1);
        auto throwing_calc = calc([](int x) -> int {
            std::cout << "Calculating with x = " << x << std::endl;
            if (x == 42) {
                throw std::runtime_error("Test exception in calculation");
            }
            return x * 2;
        }, source);
        
        std::cout << "Normal operation: " << throwing_calc.get() << std::endl;
        
        std::cout << "Setting source to 42 (this will crash the program)..." << std::endl;
        source.value(42);
        
        // This line should handle the exception, but instead crashes
        int result = throwing_calc.get();
        std::cout << "Result: " << result << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Exception caught: " << e.what() << std::endl;
    }
}

void reproduce_type_deduction_bug() {
    std::cout << "\n=== BUG 2: Expression Type Deduction Bug ===" << std::endl;
    std::cout << "Expected: 1/2 should be 0.5 in mixed-type expressions" << std::endl;
    std::cout << "Actual: Integer division gives 0" << std::endl;
    std::cout << "\nReproducing..." << std::endl;
    
    auto a = var(1);
    auto b = var(2);
    auto c = var(3.14);
    
    auto division_result = expr(a / b);
    std::cout << "a / b = " << division_result.get() << " (should be 0.5, but is " << (1/2) << ")" << std::endl;
    
    auto full_expression = expr(c + a / b - a * 2);
    std::cout << "c + a/b - a*2 = " << full_expression.get() << std::endl;
    std::cout << "Expected: " << (3.14 + 0.5 - 2) << " = 1.64" << std::endl;
    std::cout << "Actual: " << (3.14 + 0 - 2) << " = 1.14" << std::endl;
}

void reproduce_concurrent_crash() {
    std::cout << "\n=== BUG 3: Concurrent Access Crash ===" << std::endl;
    std::cout << "Expected: Thread-safe operations" << std::endl;
    std::cout << "Actual: Segmentation fault in concurrent scenarios" << std::endl;
    std::cout << "\nReproducing (this may crash)..." << std::endl;
    
    try {
        std::vector<Var<int>> sources(5);
        std::vector<Calc<int>> calculations(5);
        
        for (int i = 0; i < 5; ++i) {
            sources[i] = var(i);
            calculations[i] = calc([](int x) { return x * 2; }, sources[i]);
        }
        
        std::vector<std::future<void>> futures;
        
        // Thread 1: Modify dependencies
        futures.push_back(std::async(std::launch::async, [&]() {
            for (int i = 0; i < 100; ++i) {
                calculations[i % 5].reset([](int x) { return x * 3; }, sources[i % 5]);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }));
        
        // Thread 2: Update values
        futures.push_back(std::async(std::launch::async, [&]() {
            for (int i = 0; i < 100; ++i) {
                sources[i % 5].value(i);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }));
        
        // Thread 3: Read values
        futures.push_back(std::async(std::launch::async, [&]() {
            for (int i = 0; i < 100; ++i) {
                try {
                    int result = calculations[i % 5].get();
                    (void)result;
                } catch (...) {
                    // Ignore exceptions
                }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }));
        
        // Wait for completion
        for (auto& future : futures) {
            future.wait();
        }
        
        std::cout << "✅ Concurrent test completed (no crash this time)" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception during concurrent test: " << e.what() << std::endl;
    }
}

void reproduce_memory_corruption() {
    std::cout << "\n=== BUG 4: Memory Corruption Risk ===" << std::endl;
    std::cout << "Expected: Dangling references should be detected" << std::endl;
    std::cout << "Actual: Dangling references can be accessed" << std::endl;
    std::cout << "\nReproducing..." << std::endl;
    
    Calc<int> dangling_calc;
    
    {
        int local_value = 42;
        auto source = var(1);
        
        // This captures local_value by reference
        dangling_calc = calc([&local_value](int x) {
            return x + local_value; // Will be dangling after scope exit
        }, source);
        
        std::cout << "Initial result: " << dangling_calc.get() << std::endl;
        // local_value goes out of scope here!
    }
    
    // This accesses dangling memory
    try {
        int result = dangling_calc.get();
        std::cout << "⚠️  Dangling reference accessed: " << result << " (undefined behavior!)" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✅ Dangling reference properly detected: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Bug Reproduction Script for Reaction Framework" << std::endl;
    std::cout << "===============================================" << std::endl;
    std::cout << "\nWARNING: This script will demonstrate critical bugs." << std::endl;
    std::cout << "Some tests may crash the program!" << std::endl;
    std::cout << "\nPress Enter to continue..." << std::endl;
    std::cin.get();
    
    // Test 1: Exception crash (will likely crash the program)
    std::cout << "\n1. Testing exception handling..." << std::endl;
    try {
        reproduce_exception_crash();
    } catch (...) {
        std::cout << "Exception crash test caught at main level" << std::endl;
    }
    
    // Test 2: Type deduction bug (safe to run)
    std::cout << "\n2. Testing type deduction..." << std::endl;
    reproduce_type_deduction_bug();
    
    // Test 3: Concurrent crash (may crash)
    std::cout << "\n3. Testing concurrent access..." << std::endl;
    reproduce_concurrent_crash();
    
    // Test 4: Memory corruption (undefined behavior)
    std::cout << "\n4. Testing memory corruption..." << std::endl;
    reproduce_memory_corruption();
    
    std::cout << "\n===============================================" << std::endl;
    std::cout << "Bug reproduction completed." << std::endl;
    std::cout << "If you see this message, some bugs may not have triggered." << std::endl;
    std::cout << "Try running multiple times or under different conditions." << std::endl;
    
    return 0;
}