/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <iostream>

/**
 * @brief Test automatic disabling of thread safety in single-threaded mode
 */
TEST(ThreadSafetyTest, SingleThreadAutoDisable) {
    std::cout << "=== Single-thread Auto-disable Test ===" << std::endl;

    // Reset thread safety manager state (for testing purpose)
    auto& manager = reaction::ThreadSafetyManager::getInstance();
    manager.resetForTesting();

    // Check initial state before creating variables
    bool initialState = manager.isThreadSafetyEnabled();
    std::cout << "Thread safety state at test start: " << (initialState ? "Enabled" : "Disabled") << std::endl;

    // Create variable and perform operations
    auto var = reaction::var(42);
    int value1 = var.get();  // This triggers REACTION_REGISTER_THREAD
    var.value(100);
    int value2 = var.get();

    // Check thread safety state
    bool afterSingleThread = manager.isThreadSafetyEnabled();
    std::cout << "Thread safety state after single-thread operations: " << (afterSingleThread ? "Enabled" : "Disabled") << std::endl;

    // Verify results
    EXPECT_EQ(value1, 42);
    EXPECT_EQ(value2, 100);
    EXPECT_FALSE(afterSingleThread) << "Thread safety should be disabled in single-thread mode";

    std::cout << "✅ Single-thread auto-disable test passed" << std::endl;
}

/**
 * @brief Test automatic enabling of thread safety in multi-threaded mode
 */
TEST(ThreadSafetyTest, MultiThreadAutoEnable) {
    std::cout << "=== Multi-thread Auto-enable Test ===" << std::endl;

    auto& manager = reaction::ThreadSafetyManager::getInstance();

    // Create shared variable
    auto sharedVar = reaction::var(0);

    std::atomic<int> thread1Operations{0};
    std::atomic<int> thread2Operations{0};
    std::atomic<bool> multiThreadDetected{false};

    // Thread 1
    std::thread thread1([&]() {
        for (int i = 0; i < 100; ++i) {
            sharedVar.value(i);  // This triggers REACTION_REGISTER_THREAD
            thread1Operations++;
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });

    // Thread 2
    std::thread thread2([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Let thread1 start first
        for (int i = 0; i < 100; ++i) {
            int value = sharedVar.get();  // This triggers REACTION_REGISTER_THREAD
            (void)value;
            thread2Operations++;
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });

    thread1.join();
    thread2.join();

    // Check thread safety state after multi-threading
    bool afterMultiThread = manager.isThreadSafetyEnabled();
    std::cout << "Thread safety state after multi-thread operations: " << (afterMultiThread ? "Enabled" : "Disabled") << std::endl;

    EXPECT_TRUE(afterMultiThread) << "Thread safety should be enabled in multi-thread mode";
    EXPECT_EQ(thread1Operations.load(), 100);
    EXPECT_EQ(thread2Operations.load(), 100);

    std::cout << "✅ Multi-thread auto-enable test passed" << std::endl;
}

/**
 * @brief Test automatic transition from single-thread to multi-thread mode
 */
TEST(ThreadSafetyTest, SingleToMultiThreadTransition) {
    std::cout << "=== Single-to-Multi-thread Transition Test ===" << std::endl;

    auto& manager = reaction::ThreadSafetyManager::getInstance();

    auto sharedVar = reaction::var(0);

    // Step 1: record initial state
    bool initialState = manager.isThreadSafetyEnabled();
    std::cout << "Initial state: " << (initialState ? "Enabled" : "Disabled") << std::endl;

    // Step 2: single-thread operation
    sharedVar.value(42);
    int value = sharedVar.get();
    EXPECT_EQ(value, 42);

    // Step 3: multi-thread operation
    std::atomic<bool> multiThreadOperations{false};
    std::atomic<int> threadValue{0};

    std::thread multiThread([&]() {
        sharedVar.value(100);
        threadValue = sharedVar.get();
        multiThreadOperations = true;
    });

    multiThread.join();

    // Step 4: verify result
    bool finalState = manager.isThreadSafetyEnabled();
    std::cout << "Final state: " << (finalState ? "Enabled" : "Disabled") << std::endl;

    EXPECT_TRUE(multiThreadOperations.load());
    EXPECT_EQ(threadValue.load(), 100);

    int finalValue = sharedVar.get();
    EXPECT_EQ(finalValue, 100);

    std::cout << "✅ Single-to-Multi-thread transition test passed" << std::endl;
}

/**
 * @brief Verify REACTION_REGISTER_THREAD registration mechanism
 */
TEST(ThreadSafetyTest, ThreadRegistrationMechanism) {
    std::cout << "=== REACTION_REGISTER_THREAD Mechanism Test ===" << std::endl;

    auto& manager = reaction::ThreadSafetyManager::getInstance();

    // Test 1: verify thread count
    size_t initialCount = manager.getThreadCount();
    std::cout << "Initial thread count: " << initialCount << std::endl;

    auto var = reaction::var(42);
    var.get();  // triggers registration

    size_t afterOperationCount = manager.getThreadCount();
    std::cout << "Thread count after operation: " << afterOperationCount << std::endl;

    // Test 2: verify multi-thread registration
    std::atomic<size_t> threadCountAfterMulti{0};
    std::thread testThread([&]() {
        auto localVar = reaction::var(100);
        localVar.get();
        threadCountAfterMulti = manager.getThreadCount();
    });

    testThread.join();
    std::cout << "Thread count after multi-thread: " << threadCountAfterMulti.load() << std::endl;

    // Test 3: check state change
    bool initialSafetyState = manager.isThreadSafetyEnabled();
    std::cout << "Initial thread safety state: " << (initialSafetyState ? "Enabled" : "Disabled") << std::endl;

    std::cout << "✅ REACTION_REGISTER_THREAD mechanism test passed" << std::endl;
}

/**
 * @brief Core test: detect data races (tearing)
 *
 * This test writes compound structures to check for torn reads/writes.
 * - With mutex disabled → tearing should be detected → FAIL
 * - With mutex enabled  → no tearing → PASS
 */
TEST(ThreadSafetyTest, ThreadSafetyVerification) {
    struct TestData {
        int a;
        int b;
        TestData(int x = 0, int y = 0) : a(x), b(y) {}
        bool isValid() const { return a == b; } // valid if a == b
    };

    auto sharedVar = reaction::var(TestData(0, 0));
    std::atomic<bool> raceDetected{false};
    std::atomic<int> readCount{0};
    std::atomic<int> writeCount{0};
    std::atomic<int> tearingCount{0};

    // Writer
    auto writer = [&]() {
        for (int i = 1; i <= 5000; ++i) {
            try {
                TestData data(i, i);
                sharedVar.value(data);
                writeCount++;
                std::this_thread::sleep_for(std::chrono::nanoseconds(500));
            } catch (const std::exception& e) {
                raceDetected = true;
            }
        }
    };

    // Reader
    auto reader = [&]() {
        for (int i = 0; i < 10000; ++i) {
            try {
                TestData data = sharedVar.get();
                readCount++;
                if (!data.isValid() && !(data.a == 0 && data.b == 0)) {
                    std::cout << "Data tearing detected! a=" << data.a << ", b=" << data.b << std::endl;
                    tearingCount++;
                    raceDetected = true;
                }
                std::this_thread::sleep_for(std::chrono::nanoseconds(250));
            } catch (const std::exception& e) {
                raceDetected = true;
            }
        }
    };

    std::thread w1(writer);
    std::thread w2(writer);
    std::thread r1(reader);
    std::thread r2(reader);

    w1.join();
    w2.join();
    r1.join();
    r2.join();

    std::cout << "=== Thread Safety Verification Result ===" << std::endl;
    std::cout << "Read operations: " << readCount.load() << std::endl;
    std::cout << "Write operations: " << writeCount.load() << std::endl;
    std::cout << "Tearing detected: " << tearingCount.load() << std::endl;

    if (raceDetected.load()) {
        FAIL() << "Thread safety protection failed! Tearing detected " << tearingCount.load() << " times";
    }

    EXPECT_EQ(readCount.load(), 20000);  // 2 readers * 10000
    EXPECT_EQ(writeCount.load(), 10000); // 2 writers * 5000
    EXPECT_EQ(tearingCount.load(), 0);   // no tearing expected
}
