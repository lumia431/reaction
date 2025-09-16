/*
 * Isolated test for exception handling bug
 */

#include "reaction/reaction.h"
#include <iostream>
#include <exception>

using namespace reaction;

int main() {
    std::cout << "Testing exception handling in reactive calculations..." << std::endl;
    
    auto source = var(1);
    
    auto throwing_calc = calc([](int x) -> int {
        std::cout << "Calculation called with x = " << x << std::endl;
        if (x == 42) {
            throw std::runtime_error("Test exception in calculation");
        }
        return x * 2;
    }, source);
    
    // Test 1: Normal operation
    std::cout << "\nTest 1: Normal operation" << std::endl;
    try {
        int result = throwing_calc.get();
        std::cout << "✅ Normal result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cout << "❌ Unexpected exception: " << e.what() << std::endl;
        return 1;
    }
    
    // Test 2: Trigger exception
    std::cout << "\nTest 2: Triggering exception" << std::endl;
    source.value(42);
    
    bool exception_caught = false;
    try {
        int result = throwing_calc.get();
        std::cout << "❌ No exception thrown, result: " << result << std::endl;
    } catch (const std::runtime_error& e) {
        std::cout << "✅ Expected exception caught: " << e.what() << std::endl;
        exception_caught = true;
    } catch (const std::exception& e) {
        std::cout << "❌ Wrong exception type: " << e.what() << std::endl;
        return 1;
    }
    
    if (!exception_caught) {
        std::cout << "❌ Exception was not properly thrown/caught" << std::endl;
        return 1;
    }
    
    // Test 3: Recovery
    std::cout << "\nTest 3: Recovery from exception" << std::endl;
    source.value(5);
    
    try {
        int result = throwing_calc.get();
        std::cout << "✅ Recovery successful, result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cout << "❌ Exception during recovery: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n✅ Exception handling test completed successfully!" << std::endl;
    return 0;
}