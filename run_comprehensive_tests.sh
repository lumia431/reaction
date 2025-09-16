#!/bin/bash

# Comprehensive Testing Script for Reaction Framework
# This script runs various types of stress tests to find bugs and issues

set -e  # Exit on any error

echo "========================================="
echo "Comprehensive Reaction Framework Testing"
echo "========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "stress_tests.cpp" ]; then
    print_error "Please run this script from the directory containing stress tests"
    exit 1
fi

# Create build directory
BUILD_DIR="build_stress_tests"
print_status "Creating build directory: $BUILD_DIR"
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Build with different configurations
CONFIGURATIONS=("Debug" "Release")

for CONFIG in "${CONFIGURATIONS[@]}"; do
    print_status "Building with $CONFIG configuration"
    
    cmake .. -DCMAKE_BUILD_TYPE=$CONFIG -f ../CMakeLists_stress.txt
    make -j$(nproc)
    
    print_status "Running basic stress tests ($CONFIG)"
    ./stress_tests || print_warning "Some basic stress tests failed in $CONFIG mode"
    
    print_status "Running performance tests ($CONFIG)"
    ./stress_tests_performance || print_warning "Some performance tests failed in $CONFIG mode"
    
    print_status "Running complex scenario tests ($CONFIG)"
    ./stress_tests_complex_scenarios || print_warning "Some complex scenario tests failed in $CONFIG mode"
    
    print_status "Running type system tests ($CONFIG)"
    ./stress_tests_type_system || print_warning "Some type system tests failed in $CONFIG mode"
done

cd ..

# Memory leak detection with Valgrind (if available)
if command -v valgrind &> /dev/null; then
    print_status "Running memory leak detection with Valgrind"
    cd $BUILD_DIR
    
    valgrind --tool=memcheck \
             --leak-check=full \
             --show-leak-kinds=all \
             --track-origins=yes \
             --verbose \
             --log-file=valgrind_memory.log \
             ./stress_tests
    
    if grep -q "ERROR SUMMARY: 0 errors" valgrind_memory.log; then
        print_success "No memory errors detected"
    else
        print_warning "Memory errors detected - check valgrind_memory.log"
    fi
    
    cd ..
else
    print_warning "Valgrind not available - skipping memory leak detection"
fi

# Thread safety testing with ThreadSanitizer
print_status "Building with ThreadSanitizer"
cd $BUILD_DIR
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" -f ../CMakeLists_stress.txt
make -j$(nproc)

print_status "Running thread safety tests"
export TSAN_OPTIONS="halt_on_error=1:history_size=7"
./stress_tests || print_warning "Thread safety issues detected"

cd ..

# Address sanitizer testing
print_status "Building with AddressSanitizer"
cd $BUILD_DIR
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined -g" -f ../CMakeLists_stress.txt
make -j$(nproc)

print_status "Running AddressSanitizer tests"
export ASAN_OPTIONS="halt_on_error=1:detect_leaks=1"
export UBSAN_OPTIONS="halt_on_error=1"
./stress_tests || print_warning "Address sanitizer detected issues"

cd ..

# Stress testing with different compiler optimizations
print_status "Testing with maximum optimizations"
cd $BUILD_DIR
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native -DNDEBUG" -f ../CMakeLists_stress.txt
make -j$(nproc)

print_status "Running optimized stress tests"
./stress_tests_performance || print_warning "Performance tests failed with optimizations"

cd ..

# Long-running stress test
print_status "Running long-duration stress test"
cd $BUILD_DIR

# Run tests in background and monitor for extended period
timeout 300 ./stress_tests_performance &
STRESS_PID=$!

# Monitor memory usage
print_status "Monitoring memory usage for 5 minutes"
for i in {1..60}; do
    if ps -p $STRESS_PID > /dev/null; then
        MEMORY=$(ps -o rss= -p $STRESS_PID 2>/dev/null || echo "0")
        print_status "Memory usage: ${MEMORY}KB (iteration $i/60)"
        sleep 5
    else
        print_status "Stress test completed early"
        break
    fi
done

# Clean up
if ps -p $STRESS_PID > /dev/null; then
    kill $STRESS_PID
    print_status "Terminated long-running stress test"
fi

cd ..

# Fuzzing-style random testing
print_status "Running random/fuzzing-style tests"
cd $BUILD_DIR

# Create a simple fuzzing test
cat > random_test.cpp << 'EOF'
#include "reaction/reaction.h"
#include <random>
#include <vector>
#include <iostream>

using namespace reaction;

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);
    
    std::vector<Var<int>> vars;
    std::vector<Calc<int>> calcs;
    
    // Create random reactive network
    for (int i = 0; i < 1000; ++i) {
        vars.push_back(var(dis(gen)));
    }
    
    for (int i = 0; i < 1000; ++i) {
        std::uniform_int_distribution<> var_dis(0, vars.size() - 1);
        int idx1 = var_dis(gen);
        int idx2 = var_dis(gen);
        
        calcs.push_back(calc([](int a, int b) { 
            return (a + b) % 10000; // Prevent overflow
        }, vars[idx1], vars[idx2]));
    }
    
    // Random updates
    for (int iteration = 0; iteration < 10000; ++iteration) {
        std::uniform_int_distribution<> var_dis(0, vars.size() - 1);
        int idx = var_dis(gen);
        vars[idx].value(dis(gen));
        
        if (iteration % 1000 == 0) {
            std::cout << "Completed " << iteration << " random updates" << std::endl;
        }
    }
    
    std::cout << "Random testing completed successfully" << std::endl;
    return 0;
}
EOF

g++ -std=c++20 -I../include -O2 random_test.cpp -o random_test
./random_test || print_warning "Random testing detected issues"

cd ..

# Generate comprehensive report
print_status "Generating test report"
cat > test_report.md << EOF
# Reaction Framework Comprehensive Test Report

## Test Summary
- **Date**: $(date)
- **System**: $(uname -a)
- **Compiler**: $(g++ --version | head -n1)

## Tests Performed
1. Basic stress tests (Debug/Release)
2. Performance tests (Debug/Release)  
3. Complex scenario tests (Debug/Release)
4. Type system tests (Debug/Release)
5. Memory leak detection (Valgrind)
6. Thread safety testing (ThreadSanitizer)
7. Address sanitizer testing
8. Optimization testing
9. Long-duration stress testing
10. Random/fuzzing testing

## Log Files
- Valgrind memory report: build_stress_tests/valgrind_memory.log
- Build logs: build_stress_tests/

## Recommendations
1. Review any WARNING messages above
2. Check log files for detailed error information
3. Run tests on different architectures if possible
4. Consider additional domain-specific testing

EOF

print_success "Comprehensive testing completed!"
print_status "Check test_report.md for summary"
print_status "Check build_stress_tests/ directory for detailed logs"

# Final recommendations
echo ""
echo "========================================="
echo "TESTING RECOMMENDATIONS:"
echo "========================================="
echo "1. Run these tests on different platforms (Linux, Windows, macOS)"
echo "2. Test with different compilers (GCC, Clang, MSVC)"
echo "3. Test with different C++ standard library implementations"
echo "4. Consider property-based testing with frameworks like Catch2"
echo "5. Add domain-specific tests for your use cases"
echo "6. Set up continuous integration to run these tests regularly"
echo "7. Monitor performance regression with benchmark history"
echo "8. Test with different hardware configurations"
echo "========================================="