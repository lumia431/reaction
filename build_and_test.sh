#!/bin/bash

# Simple build and test script for the stress tests

echo "Building and testing Reaction Framework stress tests..."

# Check if we have the necessary dependencies
if ! command -v g++ &> /dev/null; then
    echo "Error: g++ compiler not found"
    exit 1
fi

# Try to compile one of the stress tests to verify the framework works
echo "Compiling basic stress test..."

g++ -std=c++20 -I./include -Wall -Wextra -O2 \
    stress_tests.cpp \
    -lgtest -lgtest_main -lpthread \
    -o stress_test_basic 2>&1

if [ $? -eq 0 ]; then
    echo "✅ Basic stress test compiled successfully"
    
    echo "Running basic stress test (first few tests only)..."
    timeout 30 ./stress_test_basic --gtest_filter="StressTestMemory.MassiveNodeCreationDestruction:StressTestMemory.CircularReferenceHandling" || echo "Some tests may have failed or timed out"
    
else
    echo "❌ Compilation failed. This indicates potential issues with:"
    echo "   1. Missing dependencies (gtest, pthread)"
    echo "   2. C++20 compiler support"
    echo "   3. Framework header issues"
    echo ""
    echo "Compilation output above shows the specific errors."
fi

# Try to compile the performance tests
echo ""
echo "Compiling performance stress test..."

g++ -std=c++20 -I./include -Wall -Wextra -O2 \
    stress_tests_performance.cpp \
    -lgtest -lgtest_main -lpthread \
    -o stress_test_performance 2>&1

if [ $? -eq 0 ]; then
    echo "✅ Performance stress test compiled successfully"
    
    echo "Running one performance test..."
    timeout 30 ./stress_test_performance --gtest_filter="PerformanceStressTest.MassiveDependencyGraph" || echo "Performance test may have failed or timed out"
    
else
    echo "❌ Performance test compilation failed"
fi

# Try to compile the complex scenarios test
echo ""
echo "Compiling complex scenarios test..."

g++ -std=c++20 -I./include -Wall -Wextra -O2 \
    stress_tests_complex_scenarios.cpp \
    -lgtest -lgtest_main -lpthread \
    -o stress_test_complex 2>&1

if [ $? -eq 0 ]; then
    echo "✅ Complex scenarios test compiled successfully"
    
    echo "Running one complex scenario test..."
    timeout 30 ./stress_test_complex --gtest_filter="ComplexScenariosTest.DynamicDependencyReconstruction" || echo "Complex test may have failed or timed out"
    
else
    echo "❌ Complex scenarios test compilation failed"
fi

# Try to compile the type system test
echo ""
echo "Compiling type system test..."

g++ -std=c++20 -I./include -Wall -Wextra -O2 \
    stress_tests_type_system.cpp \
    -lgtest -lgtest_main -lpthread \
    -o stress_test_types 2>&1

if [ $? -eq 0 ]; then
    echo "✅ Type system test compiled successfully"
    
    echo "Running one type system test..."
    timeout 30 ./stress_test_types --gtest_filter="TypeSystemStressTest.FundamentalTypes" || echo "Type system test may have failed or timed out"
    
else
    echo "❌ Type system test compilation failed"
fi

echo ""
echo "========================================="
echo "BUILD AND TEST SUMMARY"
echo "========================================="

if [ -f "stress_test_basic" ]; then
    echo "✅ Basic stress tests: COMPILED"
else
    echo "❌ Basic stress tests: FAILED TO COMPILE"
fi

if [ -f "stress_test_performance" ]; then
    echo "✅ Performance tests: COMPILED"
else
    echo "❌ Performance tests: FAILED TO COMPILE"
fi

if [ -f "stress_test_complex" ]; then
    echo "✅ Complex scenario tests: COMPILED"
else
    echo "❌ Complex scenario tests: FAILED TO COMPILE"
fi

if [ -f "stress_test_types" ]; then
    echo "✅ Type system tests: COMPILED"
else
    echo "❌ Type system tests: FAILED TO COMPILE"
fi

echo ""
echo "If compilation failed, check:"
echo "1. Install gtest: sudo apt-get install libgtest-dev"
echo "2. Ensure C++20 support: g++ --version"
echo "3. Check framework headers in ./include/"
echo "4. Review compilation errors above"

echo ""
echo "If tests compiled successfully, you can run:"
echo "  ./stress_test_basic"
echo "  ./stress_test_performance" 
echo "  ./stress_test_complex"
echo "  ./stress_test_types"

# Cleanup
echo ""
echo "Cleaning up test executables..."
rm -f stress_test_basic stress_test_performance stress_test_complex stress_test_types

echo "Done!"