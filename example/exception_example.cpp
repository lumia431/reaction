/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

/**
 * @file exception_example.cpp
 * @brief Exception handling example of the Reaction framework
 *
 * This example demonstrates:
 * - Handling various types of exceptions in reactive programming
 * - Graceful error recovery strategies
 * - Proper exception handling patterns
 * - Error propagation in reactive chains
 *
 * Use case: Safe reactive data processing with comprehensive error handling
 */

#include <reaction.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>

using namespace reaction;

void printSection(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "🚨 " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void demonstrateDependencyCycleException() {
    printSection("Dependency Cycle / Self-Observation Exception");

    std::cout << "Demonstrating both self-observation and dependency cycle detection...\n";

    // First, demonstrate self-observation
    std::cout << "\n--- Self-Observation Example ---\n";
    try {
        auto a = var(10).setName("variable_a");
        auto calcA = calc([&]() { return a() + 1; }).setName("calc_a");

        std::cout << "Attempting to create self-observation...\n";
        calcA.reset([&]() { return calcA() + a(); }); // This creates self-observation

        std::cout << "❌ No exception was thrown for self-observation!\n";

    } catch (const SelfObservationException& e) {
        std::cout << "✅ Caught SelfObservationException: " << e.what() << "\n";
    } catch (const ReactionException& e) {
        std::cout << "✅ Caught ReactionException: " << e.what() << "\n";
    }

    // Second, demonstrate dependency cycle (A -> B -> A)
    std::cout << "\n--- Dependency Cycle Example ---\n";
    try {
        auto a = var(1).setName("A");
        auto b = var(2).setName("B");

        // Create interdependent calculations
        auto calcA = calc([&]() { return b() + 1; }).setName("calc_A");
        auto calcB = calc([&]() { return a() + 1; }).setName("calc_B");

        std::cout << "Created interdependent calculations A and B\n";
        std::cout << "Current values - A: " << calcA.get() << ", B: " << calcB.get() << "\n";

        // Now try to make them truly interdependent by updating dependencies
        // This should be detected as a cycle during dependency resolution
        std::cout << "Attempting operations that might reveal cycles...\n";

        // Force evaluation and dependency tracking
        int valA = calcA.get();
        int valB = calcB.get();
        std::cout << "Evaluated values - A: " << valA << ", B: " << valB << "\n";

        // The cycle might be detected during subsequent operations
        a.value(10); // This might trigger cycle detection
        std::cout << "Successfully updated variable A\n";

    } catch (const DependencyCycleException& e) {
        std::cout << "✅ Caught DependencyCycleException: " << e.what() << "\n";
    } catch (const ReactionException& e) {
        std::cout << "✅ Caught ReactionException: " << e.what() << "\n";
    }
}

void demonstrateNullPointerAccessException() {
    printSection("Null Pointer Access Exception");

    std::cout << "Creating and then closing a reactive node...\n";

    try {
        // Create a variable
        auto myVar = var(42).setName("my_variable");
        std::cout << "Created variable with value: " << myVar.get() << "\n";

        // Close the reactive node (simulating lifecycle end)
        myVar.close();
        std::cout << "Closed the reactive node\n";

        // Try to access the closed node - this should throw
        std::cout << "Attempting to access closed node...\n";
        int value = myVar.get();
        std::cout << "❌ No exception was thrown - value: " << value << "\n";

    } catch (const NullPointerAccessException& e) {
        std::cout << "✅ Caught NullPointerAccessException: " << e.what() << "\n";
    } catch (const ReactionException& e) {
        std::cout << "✅ Caught ReactionException: " << e.what() << "\n";
    }
}

void demonstrateTypeMismatchException() {
    printSection("Type Mismatch Exception");

    std::cout << "Attempting type-related errors in reactive operations...\n";

    try {
        // Create calculations with different return types
        auto intCalc = calc([]() { return 42; }).setName("int_calculation");
        auto stringCalc = calc([]() { return std::string("hello"); }).setName("string_calculation");

        std::cout << "Created int calculation: " << intCalc.get() << "\n";
        std::cout << "Created string calculation: " << stringCalc.get() << "\n";

        // In C++, direct type mismatch would be caught at compile time
        // But runtime type errors can occur during complex operations
        std::cout << "Note: Direct type mismatches are caught at compile time in C++\n";
        std::cout << "TypeMismatchException would occur during runtime type operations\n";
        std::cout << "or in dynamic typing scenarios.\n";

        // Example: This would cause compile error, not runtime exception
        // intCalc.reset([]() { return std::string("hello"); });

    } catch (const TypeMismatchException& e) {
        std::cout << "✅ Caught TypeMismatchException: " << e.what() << "\n";
    } catch (const ReactionException& e) {
        std::cout << "✅ Caught ReactionException: " << e.what() << "\n";
    }
}

void demonstrateErrorRecovery() {
    printSection("Error Recovery and Graceful Handling");

    std::cout << "Demonstrating error recovery patterns...\n";

    // Example 1: Safe variable access with error recovery
    auto safeVar = var(100).setName("safe_variable");

    auto safeAccessor = action([&]() {
        try {
            int value = safeVar.get();
            std::cout << "✅ Successfully accessed safe variable: " << value << "\n";
        } catch (const NullPointerAccessException& e) {
            std::cout << "⚠️  Variable is no longer available: " << e.getContext() << "\n";
            std::cout << "    Continuing with default value...\n";
        } catch (const ReactionException& e) {
            std::cout << "⚠️  Reaction error: " << e.what() << "\n";
        }
    });

    // Test normal operation
    std::cout << "Testing normal operation:\n";
    safeVar.value(200);

    // Test error recovery
    std::cout << "\nTesting error recovery after closing variable:\n";
    safeVar.close();

    // The action should handle the error gracefully
    std::cout << "Action should handle the closed variable gracefully...\n";
}

void demonstrateBatchOperationConflicts() {
    printSection("Batch Operation Conflicts Exception");

    std::cout << "Demonstrating batch operation conflict detection...\n";

    auto counter = var(0).setName("counter");
    auto tempCalc = calc([&]() { return counter() * 2; }).setName("temp_calc");

    std::cout << "Initial tempCalc value: " << tempCalc.get() << "\n";

    // Test batch operation conflict - reset operations during batch execution throw exceptions
    std::cout << "\nTesting batch operation conflict (reset within batch):\n";
    try {
        batchExecute([&]() {
            counter.value(1);
            std::cout << "Updated counter within batch\n";

            // Reset within batch - this should throw BatchOperationConflictException
            std::cout << "Attempting to reset calculation within active batch...\n";
            tempCalc.reset([&]() { return counter() + 10; });
        });

        std::cout << "Batch operation completed\n";

    } catch (const BatchOperationConflictException& e) {
        std::cout << "✅ Caught BatchOperationConflictException: " << e.what() << "\n";
    } catch (const ReactionException& e) {
        std::cout << "✅ Caught ReactionException: " << e.what() << "\n";
    }

    // Test normal batch operation (no conflicts)
    std::cout << "\nTesting normal batch operation (no conflicts):\n";
    try {
        batchExecute([&]() {
            counter.value(5);
            std::cout << "✅ Normal batch operation succeeded - tempCalc: " << tempCalc.get() << "\n";
        });
        std::cout << "Batch operation completed successfully\n";
        std::cout << "Final tempCalc value: " << tempCalc.get() << "\n";
    } catch (const ReactionException& e) {
        std::cout << "⚠️  Unexpected exception in normal batch: " << e.what() << "\n";
    }
}

void demonstrateExceptionPropagation() {
    printSection("Exception Propagation in Reactive Chains");

    std::cout << "Demonstrating how exceptions propagate through reactive chains...\n";

    try {
        // Create a chain: var -> calc -> action
        auto source = var(10).setName("source");
        auto multiplier = calc([&]() {
            int val = source();
            if (val < 0) {
                throw std::runtime_error("Negative values not allowed in multiplier");
            }
            return val * 2;
        }).setName("multiplier");

        auto logger = action([&]() {
            try {
                std::cout << "Multiplier result: " << multiplier() << "\n";
            } catch (const std::runtime_error& e) {
                std::cout << "⚠️  Runtime error in action: " << e.what() << "\n";
                throw; // Re-throw to demonstrate propagation
            }
        }).setName("logger");

        // Test normal operation
        std::cout << "Testing normal operation:\n";
        source.value(5);

        // Test error propagation
        std::cout << "\nTesting error propagation:\n";
        source.value(-1);

    } catch (const ReactionException& e) {
        std::cout << "✅ Caught ReactionException: " << e.what() << "\n";
    } catch (const std::runtime_error& e) {
        std::cout << "✅ Caught runtime_error: " << e.what() << "\n";
    }
}

int main() {
    std::cout << "🛡️  Reaction Framework - Exception Handling Examples\n";
    std::cout << "Demonstrating robust error handling in reactive programming\n";

    // Demonstrate different types of exceptions
    demonstrateDependencyCycleException();
    demonstrateNullPointerAccessException();
    demonstrateTypeMismatchException();
    demonstrateErrorRecovery();
    demonstrateBatchOperationConflicts();
    demonstrateExceptionPropagation();

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "✅ Exception handling examples completed!\n";
    std::cout << "🛡️  Always handle exceptions appropriately in production code.\n";
    std::cout << std::string(60, '=') << "\n";

    return 0;
}
