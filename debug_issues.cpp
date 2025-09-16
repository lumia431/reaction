/*
 * Debug Issues Found in Reaction Framework
 * 
 * This file isolates and debugs the specific issues found during stress testing.
 */

#include "reaction/reaction.h"
#include <iostream>
#include <exception>

using namespace reaction;

void debug_exception_handling() {
    std::cout << "=== Debugging Exception Handling ===" << std::endl;
    
    auto source = var(1);
    
    auto throwing_calc = calc([](int x) -> int {
        std::cout << "Calculating with x = " << x << std::endl;
        if (x == 42) {
            std::cout << "About to throw exception" << std::endl;
            throw std::runtime_error("Test exception");
        }
        return x * 2;
    }, source);
    
    std::cout << "Initial calculation:" << std::endl;
    try {
        int result = throwing_calc.get();
        std::cout << "Result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Exception caught: " << e.what() << std::endl;
    }
    
    std::cout << "\nSetting source to 42 (should throw):" << std::endl;
    source.value(42);
    
    try {
        int result = throwing_calc.get();
        std::cout << "Result: " << result << " (This shouldn't happen!)" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Exception caught as expected: " << e.what() << std::endl;
    }
    
    std::cout << "\nSetting source to 10 (should recover):" << std::endl;
    source.value(10);
    
    try {
        int result = throwing_calc.get();
        std::cout << "Result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Unexpected exception during recovery: " << e.what() << std::endl;
    }
}

void debug_expression_templates() {
    std::cout << "\n=== Debugging Expression Templates ===" << std::endl;
    
    auto a = var(1);
    auto b = var(2);
    auto c = var(3.14);
    
    std::cout << "Variables: a=" << a.get() << ", b=" << b.get() << ", c=" << c.get() << std::endl;
    
    // Let's break down the expression step by step
    auto a_div_b = expr(a / b);
    auto a_mul_2 = expr(a * 2);
    auto c_plus_div = expr(c + a_div_b);
    auto final_expr = expr(c_plus_div - a_mul_2);
    
    std::cout << "Step by step:" << std::endl;
    std::cout << "a / b = " << a_div_b.get() << std::endl;
    std::cout << "a * 2 = " << a_mul_2.get() << std::endl;
    std::cout << "c + (a/b) = " << c_plus_div.get() << std::endl;
    std::cout << "c + (a/b) - (a*2) = " << final_expr.get() << std::endl;
    
    // Now test the combined expression
    auto combined_expr = expr(c + a / b - a * 2);
    std::cout << "Combined expression result: " << combined_expr.get() << std::endl;
    
    // Expected calculation:
    // c + a/b - a*2 = 3.14 + 1/2 - 1*2 = 3.14 + 0.5 - 2 = 1.64
    double expected = 3.14 + 1.0/2.0 - 1*2;
    std::cout << "Expected result: " << expected << std::endl;
    std::cout << "Difference: " << (combined_expr.get() - expected) << std::endl;
    
    // Test with integer division
    std::cout << "\nTesting integer division:" << std::endl;
    std::cout << "1 / 2 (int division) = " << (1 / 2) << std::endl;
    std::cout << "1.0 / 2.0 (double division) = " << (1.0 / 2.0) << std::endl;
    
    // Test operator precedence
    std::cout << "\nTesting operator precedence:" << std::endl;
    auto prec_test1 = expr(a + b * c);
    std::cout << "a + b * c = " << prec_test1.get() << " (should be " << (1 + 2 * 3.14) << ")" << std::endl;
    
    auto prec_test2 = expr((a + b) * c);
    std::cout << "(a + b) * c = " << prec_test2.get() << " (should be " << ((1 + 2) * 3.14) << ")" << std::endl;
}

void debug_batch_operations() {
    std::cout << "\n=== Debugging Batch Operations ===" << std::endl;
    
    auto a = var(1);
    auto b = var(2);
    
    int trigger_count = 0;
    auto ds = calc([&trigger_count](int aa, int bb) { 
        trigger_count++;
        std::cout << "Calculation triggered with a=" << aa << ", b=" << bb << ", count=" << trigger_count << std::endl;
        return aa + bb; 
    }, a, b);
    
    std::cout << "Initial calculation:" << std::endl;
    std::cout << "Result: " << ds.get() << ", trigger count: " << trigger_count << std::endl;
    
    trigger_count = 0;
    std::cout << "\nIndividual updates:" << std::endl;
    a.value(10);
    std::cout << "After a.value(10): result=" << ds.get() << ", trigger count=" << trigger_count << std::endl;
    
    b.value(20);
    std::cout << "After b.value(20): result=" << ds.get() << ", trigger count=" << trigger_count << std::endl;
    
    trigger_count = 0;
    std::cout << "\nBatch update:" << std::endl;
    batchExecute([&]() {
        std::cout << "Inside batch: setting a=100, b=200" << std::endl;
        a.value(100);
        std::cout << "Set a=100, trigger count so far: " << trigger_count << std::endl;
        b.value(200);
        std::cout << "Set b=200, trigger count so far: " << trigger_count << std::endl;
    });
    
    std::cout << "After batch: result=" << ds.get() << ", trigger count=" << trigger_count << std::endl;
}

void debug_move_semantics() {
    std::cout << "\n=== Debugging Move Semantics ===" << std::endl;
    
    auto original = var(42);
    std::cout << "Original var created with value: " << original.get() << std::endl;
    std::cout << "Original is valid: " << static_cast<bool>(original) << std::endl;
    
    auto moved = std::move(original);
    std::cout << "After move:" << std::endl;
    std::cout << "Moved is valid: " << static_cast<bool>(moved) << std::endl;
    std::cout << "Original is valid: " << static_cast<bool>(original) << std::endl;
    
    if (moved) {
        std::cout << "Moved value: " << moved.get() << std::endl;
    }
    
    if (original) {
        std::cout << "Original value: " << original.get() << std::endl;
    } else {
        std::cout << "Original is invalid (as expected)" << std::endl;
        try {
            original.get();
            std::cout << "ERROR: original.get() should have thrown!" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "original.get() correctly threw: " << e.what() << std::endl;
        }
    }
}

int main() {
    std::cout << "Debugging Issues Found in Reaction Framework" << std::endl;
    std::cout << "============================================" << std::endl;
    
    try {
        debug_exception_handling();
        debug_expression_templates();
        debug_batch_operations();
        debug_move_semantics();
    } catch (const std::exception& e) {
        std::cout << "\nUnexpected exception during debugging: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nDebugging completed." << std::endl;
    return 0;
}