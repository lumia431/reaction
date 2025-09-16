/*
 * Isolated test for expression template calculation bug
 */

#include "reaction/reaction.h"
#include <iostream>
#include <cmath>

using namespace reaction;

int main() {
    std::cout << "Testing expression template calculations..." << std::endl;
    
    auto a = var(1);
    auto b = var(2);
    auto c = var(3.14);
    
    std::cout << "Variables: a=" << a.get() << ", b=" << b.get() << ", c=" << c.get() << std::endl;
    
    // Test individual operations
    std::cout << "\nTesting individual operations:" << std::endl;
    
    auto test1 = expr(a / b);
    std::cout << "a / b = " << test1.get() << " (expected: " << (1.0/2.0) << ")" << std::endl;
    std::cout << "Note: This might be integer division! 1/2 = " << (1/2) << std::endl;
    
    auto test2 = expr(a * 2);
    std::cout << "a * 2 = " << test2.get() << " (expected: " << (1*2) << ")" << std::endl;
    
    auto test3 = expr(c + test1);
    std::cout << "c + (a/b) = " << test3.get() << " (expected: " << (3.14 + (1.0/2.0)) << ")" << std::endl;
    
    // Test the full expression
    std::cout << "\nTesting full expression:" << std::endl;
    auto full_expr = expr(c + a / b - a * 2);
    double result = full_expr.get();
    std::cout << "c + a / b - a * 2 = " << result << std::endl;
    
    // Calculate expected result manually
    std::cout << "\nManual calculation:" << std::endl;
    std::cout << "c = " << 3.14 << std::endl;
    std::cout << "a / b = " << (1.0/2.0) << " (if double division)" << std::endl;
    std::cout << "a / b = " << (1/2) << " (if integer division)" << std::endl;
    std::cout << "a * 2 = " << (1*2) << std::endl;
    
    double expected_double = 3.14 + (1.0/2.0) - (1*2);
    double expected_int = 3.14 + (1/2) - (1*2);
    
    std::cout << "Expected (double division): " << expected_double << std::endl;
    std::cout << "Expected (integer division): " << expected_int << std::endl;
    std::cout << "Actual result: " << result << std::endl;
    
    std::cout << "\nDifference from double division: " << std::abs(result - expected_double) << std::endl;
    std::cout << "Difference from integer division: " << std::abs(result - expected_int) << std::endl;
    
    // Test type promotion
    std::cout << "\nTesting type promotion:" << std::endl;
    auto int_var = var(1);
    auto double_var = var(2.0);
    auto mixed_expr = expr(int_var / double_var);
    std::cout << "int(1) / double(2.0) = " << mixed_expr.get() << std::endl;
    
    return 0;
}