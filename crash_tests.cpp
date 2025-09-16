/*
 * Crash Tests and Extreme Edge Cases for Reaction Framework
 * 
 * These tests are designed to find crashes, undefined behavior,
 * and other serious issues that might not be caught by regular tests.
 * 
 * WARNING: Some of these tests may intentionally cause crashes
 * or consume large amounts of memory. Run with caution!
 */

#include "reaction/reaction.h"
#include "gtest/gtest.h"
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <random>
#include <limits>
#include <signal.h>
#include <setjmp.h>

using namespace reaction;

class CrashTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up signal handlers for crash detection
        signal(SIGSEGV, signal_handler);
        signal(SIGABRT, signal_handler);
    }
    
    void TearDown() override {
        // Reset signal handlers
        signal(SIGSEGV, SIG_DFL);
        signal(SIGABRT, SIG_DFL);
    }
    
    static jmp_buf jump_buffer;
    static bool signal_caught;
    
    static void signal_handler(int sig) {
        signal_caught = true;
        longjmp(jump_buffer, sig);
    }
};

jmp_buf CrashTest::jump_buffer;
bool CrashTest::signal_caught = false;

// Test extremely deep recursion
TEST_F(CrashTest, ExtremelyDeepRecursion) {
    constexpr int EXTREME_DEPTH = 100000;
    
    if (setjmp(jump_buffer) == 0) {
        auto root = var(1);
        std::vector<Calc<int>> chain;
        chain.reserve(EXTREME_DEPTH);
        
        // This might cause stack overflow
        chain.push_back(calc([](int x) { return x + 1; }, root));
        for (int i = 1; i < EXTREME_DEPTH; ++i) {
            chain.push_back(calc([](int x) { return x + 1; }, chain[i-1]));
        }
        
        root.value(100);
        int result = chain[EXTREME_DEPTH-1].get();
        EXPECT_EQ(result, 100 + EXTREME_DEPTH);
        
        if (signal_caught) {
            FAIL() << "Signal caught during extreme recursion test";
        }
    } else {
        // Signal was caught
        EXPECT_TRUE(signal_caught);
        std::cout << "Stack overflow detected and handled" << std::endl;
    }
}

// Test massive memory allocation
TEST_F(CrashTest, MassiveMemoryAllocation) {
    // Try to allocate huge number of nodes
    constexpr int HUGE_COUNT = 1000000;
    
    try {
        std::vector<Var<std::vector<int>>> huge_vars;
        huge_vars.reserve(HUGE_COUNT);
        
        for (int i = 0; i < HUGE_COUNT; ++i) {
            // Each var contains a vector with many elements
            std::vector<int> large_vector(1000, i);
            huge_vars.push_back(var(std::move(large_vector)));
            
            // Check if we're running out of memory
            if (i % 10000 == 0) {
                std::cout << "Created " << i << " large variables" << std::endl;
            }
        }
        
        // Try to use some of the variables
        for (int i = 0; i < std::min(1000, HUGE_COUNT); ++i) {
            EXPECT_EQ(huge_vars[i].get().size(), 1000);
        }
        
    } catch (const std::bad_alloc& e) {
        std::cout << "Out of memory as expected: " << e.what() << std::endl;
        // This is expected behavior
    }
}

// Test circular dependencies with many nodes
TEST_F(CrashTest, MassiveCircularDependencies) {
    constexpr int CIRCLE_SIZE = 1000;
    
    std::vector<Calc<int>> circle_nodes;
    circle_nodes.reserve(CIRCLE_SIZE);
    
    auto source = var(1);
    
    // Create initial nodes
    for (int i = 0; i < CIRCLE_SIZE; ++i) {
        circle_nodes.push_back(calc([i](int x) { return x + i; }, source));
    }
    
    // Try to create circular dependencies (should throw)
    for (int i = 0; i < CIRCLE_SIZE; ++i) {
        int next = (i + 1) % CIRCLE_SIZE;
        EXPECT_THROW({
            circle_nodes[i].reset([&circle_nodes, next](int x) {
                return x + circle_nodes[next]();
            }, source);
        }, std::runtime_error);
    }
}

// Test rapid creation and destruction
TEST_F(CrashTest, RapidCreationDestruction) {
    constexpr int ITERATIONS = 10000;
    constexpr int NODES_PER_ITERATION = 100;
    
    for (int iter = 0; iter < ITERATIONS; ++iter) {
        std::vector<Var<int>> temp_vars;
        std::vector<Calc<int>> temp_calcs;
        
        temp_vars.reserve(NODES_PER_ITERATION);
        temp_calcs.reserve(NODES_PER_ITERATION);
        
        // Rapid creation
        for (int i = 0; i < NODES_PER_ITERATION; ++i) {
            temp_vars.push_back(var(i));
            temp_calcs.push_back(calc([](int x) { return x * 2; }, temp_vars[i]));
        }
        
        // Rapid usage
        for (int i = 0; i < NODES_PER_ITERATION; ++i) {
            temp_vars[i].value(i + iter);
            int result = temp_calcs[i].get();
            EXPECT_EQ(result, (i + iter) * 2);
        }
        
        // Nodes are destroyed at end of scope
        
        if (iter % 1000 == 0) {
            std::cout << "Completed " << iter << " rapid creation/destruction cycles" << std::endl;
        }
    }
}

// Test concurrent crashes
TEST_F(CrashTest, ConcurrentCrashConditions) {
    constexpr int THREAD_COUNT = 16;
    constexpr int OPERATIONS_PER_THREAD = 1000;
    
    std::vector<Var<int>> shared_vars(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; ++i) {
        shared_vars[i] = var(i);
    }
    
    std::atomic<int> crash_count{0};
    std::vector<std::future<void>> futures;
    
    // Launch aggressive concurrent operations
    for (int t = 0; t < THREAD_COUNT; ++t) {
        futures.push_back(std::async(std::launch::async, [t, &shared_vars, &crash_count]() {
            try {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> var_dis(0, THREAD_COUNT - 1);
                
                for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                    // Random operations
                    int var_idx = var_dis(gen);
                    
                    // Create temporary calculations
                    auto temp_calc = calc([t, i](int x) { 
                        return x + t * 1000 + i; 
                    }, shared_vars[var_idx]);
                    
                    // Update value
                    shared_vars[var_idx].value(shared_vars[var_idx].get() + 1);
                    
                    // Get result
                    int result = temp_calc.get();
                    (void)result; // Suppress unused warning
                    
                    // Reset calculation
                    temp_calc.reset([t, i](int x) { 
                        return x * 2 + t * 1000 + i; 
                    }, shared_vars[var_idx]);
                    
                    // temp_calc goes out of scope
                }
            } catch (...) {
                crash_count++;
            }
        }));
    }
    
    // Wait for all threads
    for (auto& future : futures) {
        future.wait();
    }
    
    EXPECT_EQ(crash_count.load(), 0) << "Concurrent operations caused crashes";
}

// Test invalid memory access patterns
TEST_F(CrashTest, InvalidMemoryAccessPatterns) {
    // Test use after move
    {
        auto original = var(42);
        auto moved = std::move(original);
        
        EXPECT_TRUE(static_cast<bool>(moved));
        EXPECT_FALSE(static_cast<bool>(original));
        
        // This should throw
        EXPECT_THROW(original.get(), std::runtime_error);
        
        // This should work
        EXPECT_EQ(moved.get(), 42);
    }
    
    // Test dangling references
    {
        std::vector<Calc<int>> dangling_calcs;
        
        {
            auto temp_var = var(100);
            dangling_calcs.push_back(calc([](int x) { return x * 2; }, temp_var));
            EXPECT_EQ(dangling_calcs[0].get(), 200);
            // temp_var goes out of scope
        }
        
        // This should still work due to KeepStrategy (default)
        EXPECT_NO_THROW(dangling_calcs[0].get());
        EXPECT_EQ(dangling_calcs[0].get(), 200);
    }
}

// Test numeric overflow and underflow
TEST_F(CrashTest, NumericOverflowUnderflow) {
    auto max_int = var(std::numeric_limits<int>::max());
    auto min_int = var(std::numeric_limits<int>::min());
    
    // Test overflow
    auto overflow_calc = calc([](int x) {
        return x + 1; // This will overflow
    }, max_int);
    
    // Should not crash, but result is undefined
    EXPECT_NO_THROW(overflow_calc.get());
    
    // Test underflow
    auto underflow_calc = calc([](int x) {
        return x - 1; // This will underflow
    }, min_int);
    
    EXPECT_NO_THROW(underflow_calc.get());
    
    // Test multiplication overflow
    auto mult_overflow = calc([](int x) {
        return x * x; // This will definitely overflow
    }, max_int);
    
    EXPECT_NO_THROW(mult_overflow.get());
    
    // Test division by zero (with integers)
    auto zero_var = var(0);
    auto div_by_zero = calc([](int x) {
        return 100 / x; // Division by zero
    }, zero_var);
    
    // This might crash or throw, depending on implementation
    try {
        int result = div_by_zero.get();
        (void)result;
        std::cout << "Division by zero did not crash" << std::endl;
    } catch (...) {
        std::cout << "Division by zero threw exception" << std::endl;
    }
}

// Test exception propagation in complex scenarios
TEST_F(CrashTest, ExceptionPropagationCrash) {
    auto source = var(1);
    
    // Create chain of calculations that might throw
    auto calc1 = calc([](int x) -> int {
        if (x == 666) throw std::runtime_error("Devil's number");
        return x * 2;
    }, source);
    
    auto calc2 = calc([](int x) -> int {
        if (x == 1334) throw std::logic_error("Logic error");
        return x + 10;
    }, calc1);
    
    auto calc3 = calc([](int x) -> std::string {
        if (x == 1344) throw std::invalid_argument("Invalid argument");
        return std::to_string(x);
    }, calc2);
    
    // Normal operation
    EXPECT_EQ(calc3.get(), "12"); // (1*2)+10 = 12
    
    // Trigger first exception
    source.value(666);
    EXPECT_THROW(calc1.get(), std::runtime_error);
    EXPECT_THROW(calc2.get(), std::runtime_error); // Should propagate
    EXPECT_THROW(calc3.get(), std::runtime_error); // Should propagate
    
    // Reset to normal
    source.value(1);
    EXPECT_EQ(calc3.get(), "12");
    
    // Trigger second exception
    source.value(667); // 667*2 = 1334
    EXPECT_EQ(calc1.get(), 1334);
    EXPECT_THROW(calc2.get(), std::logic_error);
    EXPECT_THROW(calc3.get(), std::logic_error);
    
    // Trigger third exception
    source.value(667); // 667*2+10 = 1344
    calc2.reset([](int x) { return x + 10; }, calc1); // Remove exception
    EXPECT_EQ(calc2.get(), 1344);
    EXPECT_THROW(calc3.get(), std::invalid_argument);
}

// Test resource exhaustion
TEST_F(CrashTest, ResourceExhaustion) {
    // Test thread exhaustion
    std::vector<std::thread> threads;
    std::atomic<bool> stop_flag{false};
    
    try {
        // Create many threads
        for (int i = 0; i < 1000; ++i) {
            threads.emplace_back([&stop_flag, i]() {
                auto local_var = var(i);
                auto local_calc = calc([](int x) { return x * 2; }, local_var);
                
                while (!stop_flag.load()) {
                    local_var.value(local_var.get() + 1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
        }
        
        // Let them run for a bit
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Stop all threads
        stop_flag = true;
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
    } catch (const std::system_error& e) {
        std::cout << "Thread creation failed (expected): " << e.what() << std::endl;
        
        // Clean up any created threads
        stop_flag = true;
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
}

// Test with corrupted data
TEST_F(CrashTest, CorruptedDataHandling) {
    // Test with uninitialized memory patterns
    struct PotentiallyCorrupted {
        int* ptr;
        size_t size;
        
        PotentiallyCorrupted() : ptr(nullptr), size(0) {}
        
        PotentiallyCorrupted(size_t s) : size(s) {
            ptr = new int[s];
            for (size_t i = 0; i < s; ++i) {
                ptr[i] = static_cast<int>(i);
            }
        }
        
        ~PotentiallyCorrupted() {
            delete[] ptr;
        }
        
        // Copy constructor
        PotentiallyCorrupted(const PotentiallyCorrupted& other) : size(other.size) {
            if (size > 0) {
                ptr = new int[size];
                std::copy(other.ptr, other.ptr + size, ptr);
            } else {
                ptr = nullptr;
            }
        }
        
        // Assignment operator
        PotentiallyCorrupted& operator=(const PotentiallyCorrupted& other) {
            if (this != &other) {
                delete[] ptr;
                size = other.size;
                if (size > 0) {
                    ptr = new int[size];
                    std::copy(other.ptr, other.ptr + size, ptr);
                } else {
                    ptr = nullptr;
                }
            }
            return *this;
        }
        
        bool operator==(const PotentiallyCorrupted& other) const {
            if (size != other.size) return false;
            if (size == 0) return true;
            return std::equal(ptr, ptr + size, other.ptr);
        }
        
        int sum() const {
            int total = 0;
            for (size_t i = 0; i < size; ++i) {
                total += ptr[i];
            }
            return total;
        }
    };
    
    auto corrupted_var = var(PotentiallyCorrupted(100));
    auto corrupted_calc = calc([](const PotentiallyCorrupted& pc) {
        return pc.sum();
    }, corrupted_var);
    
    EXPECT_NO_THROW(corrupted_calc.get());
    
    // Test with different sizes
    for (size_t size : {0, 1, 10, 100, 1000}) {
        corrupted_var.value(PotentiallyCorrupted(size));
        EXPECT_NO_THROW(corrupted_calc.get());
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "WARNING: These tests may consume large amounts of memory" << std::endl;
    std::cout << "WARNING: Some tests may intentionally trigger crashes" << std::endl;
    std::cout << "Press Enter to continue or Ctrl+C to abort..." << std::endl;
    std::cin.get();
    
    return RUN_ALL_TESTS();
}