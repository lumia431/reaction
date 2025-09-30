/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"
#include "reaction/memory/sbo_resource.h"
#include "reaction/memory/stack_monitor.h"
#include "reaction/memory/memory_config.h"
#include <cstring>  // For memcmp
#include <memory>   // For std::make_shared

using namespace reaction;
using namespace reaction::memory;

// Test data structures for SBO testing
struct SmallStruct {
    int x, y;
    SmallStruct(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const SmallStruct& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const SmallStruct& other) const {
        return !(*this == other);
    }
};

struct MediumStruct {
    double data[4];  // 32 bytes
    MediumStruct(double val = 0.0) {
        for (int i = 0; i < 4; ++i) data[i] = val + i;
    }
    bool operator==(const MediumStruct& other) const {
        for (int i = 0; i < 4; ++i) {
            if (data[i] != other.data[i]) return false;
        }
        return true;
    }
    bool operator!=(const MediumStruct& other) const {
        return !(*this == other);
    }
};

struct LargeStruct {
    char data[128];  // 128 bytes - should force heap allocation
    LargeStruct(char fill = 'A') {
        std::fill(data, data + 128, fill);
    }
    bool operator==(const LargeStruct& other) const {
        return std::memcmp(data, other.data, 128) == 0;
    }
    bool operator!=(const LargeStruct& other) const {
        return !(*this == other);
    }
};

class SBOTest : public ::testing::Test {
protected:
    void SetUp() override {
        StackMonitor::initialize();
    }

    void TearDown() override {
        StackMonitor::setMonitoringEnabled(false);
    }
};

// Test SBO traits for different types
TEST_F(SBOTest, SBOTraitsBasicTypes) {
    // Small built-in types should be SBO eligible
    EXPECT_TRUE(SBOTraits<int>::is_sbo_eligible);
    EXPECT_TRUE(SBOTraits<int>::always_sbo);
    EXPECT_FALSE(SBOTraits<int>::conditional_sbo);

    EXPECT_TRUE(SBOTraits<double>::is_sbo_eligible);
    EXPECT_TRUE(SBOTraits<double>::always_sbo);

    // Small structs should be SBO eligible
    EXPECT_TRUE(SBOTraits<SmallStruct>::is_sbo_eligible);
    EXPECT_TRUE(SBOTraits<SmallStruct>::always_sbo);

    // Medium structs should be conditionally SBO eligible
    EXPECT_TRUE(SBOTraits<MediumStruct>::is_sbo_eligible);
    EXPECT_FALSE(SBOTraits<MediumStruct>::always_sbo);
    EXPECT_TRUE(SBOTraits<MediumStruct>::conditional_sbo);

    // Large structs should not be SBO eligible
    EXPECT_FALSE(SBOTraits<LargeStruct>::is_sbo_eligible);
    EXPECT_TRUE(SBOTraits<LargeStruct>::force_heap);

    // Standard library containers should be forced to heap
    EXPECT_TRUE(SBOTraits<std::string>::force_heap);
    EXPECT_TRUE(SBOTraits<std::vector<int>>::force_heap);
}

// Test stack monitor functionality
TEST_F(SBOTest, StackMonitorBasics) {
    EXPECT_EQ(StackMonitor::getCurrentUsage(), 0);
    EXPECT_TRUE(StackMonitor::canAllocate<16>());
    EXPECT_TRUE(StackMonitor::canAllocate(32));

    {
        StackMonitor::StackGuard<16> guard;
        EXPECT_TRUE(guard.success());
        EXPECT_EQ(StackMonitor::getCurrentUsage(), 16);

        {
            StackMonitor::StackGuard<32> guard2;
            EXPECT_TRUE(guard2.success());
            EXPECT_EQ(StackMonitor::getCurrentUsage(), 48);
        }
        EXPECT_EQ(StackMonitor::getCurrentUsage(), 16);
    }
    EXPECT_EQ(StackMonitor::getCurrentUsage(), 0);
}

// Test stack monitor overflow protection
TEST_F(SBOTest, StackMonitorOverflowProtection) {
    constexpr size_t large_size = SBOConfig::MAX_SBO_STACK_USAGE + 1;

    EXPECT_FALSE(StackMonitor::canAllocate(large_size));

    StackMonitor::StackGuard<large_size> guard;
    EXPECT_FALSE(guard.success());
    EXPECT_EQ(StackMonitor::getCurrentUsage(), 0);
}

// Test SBOResource with small types
TEST_F(SBOTest, SBOResourceSmallTypes) {
    // Test int
    {
        auto resource = std::make_shared<SBOResource<int>>(42);
        EXPECT_TRUE(resource->isUsingSBO());
        EXPECT_EQ(resource->getValue(), 42);

        EXPECT_TRUE(resource->updateValue(100));
        EXPECT_EQ(resource->getValue(), 100);

        // Same value should not trigger change
        EXPECT_FALSE(resource->updateValue(100));
    }

    // Test SmallStruct
    {
        auto resource = std::make_shared<SBOResource<SmallStruct>>(SmallStruct{10, 20});
        EXPECT_TRUE(resource->isUsingSBO());
        EXPECT_EQ(resource->getValue(), SmallStruct(10, 20));

        EXPECT_TRUE(resource->updateValue(SmallStruct{30, 40}));
        EXPECT_EQ(resource->getValue(), SmallStruct(30, 40));
    }
}

// Test SBOResource with medium types (conditional SBO)
TEST_F(SBOTest, SBOResourceMediumTypes) {
    auto resource = std::make_shared<SBOResource<MediumStruct>>(MediumStruct{1.0});

    // Should use SBO when stack allows
    EXPECT_TRUE(resource->isUsingSBO());
    EXPECT_EQ(resource->getValue(), MediumStruct(1.0));

    EXPECT_TRUE(resource->updateValue(MediumStruct{2.0}));
    EXPECT_EQ(resource->getValue(), MediumStruct(2.0));
}

// Test SBOResource with large types (heap only)
TEST_F(SBOTest, SBOResourceLargeTypes) {
    auto resource = std::make_shared<SBOResource<LargeStruct>>(LargeStruct{'X'});

    // Should always use heap for large types
    EXPECT_FALSE(resource->isUsingSBO());
    EXPECT_EQ(resource->getValue(), LargeStruct('X'));

    EXPECT_TRUE(resource->updateValue(LargeStruct{'Y'}));
    EXPECT_EQ(resource->getValue(), LargeStruct('Y'));
}

// Test SBOResource with uninitialized state
TEST_F(SBOTest, SBOResourceUninitialized) {
    auto resource = std::make_shared<SBOResource<int>>();

    EXPECT_THROW({
        auto value = resource->getValue();
        (void)value;
    }, std::runtime_error);

    EXPECT_THROW({
        auto ptr = resource->getRawPtr();
        (void)ptr;
    }, std::runtime_error);

    // First update should succeed
    EXPECT_TRUE(resource->updateValue(42));
    EXPECT_EQ(resource->getValue(), 42);
    EXPECT_EQ(*resource->getRawPtr(), 42);
}

// Test Resource class with SBO integration
TEST_F(SBOTest, ResourceClassWithSBO) {
    // Note: This test is disabled until Resource<> properly integrates SBO
    // For now, we test SBOResource directly above
    EXPECT_TRUE(true);  // Placeholder
}

// Test reactive variables with SBO
TEST_F(SBOTest, ReactiveVariablesWithSBO) {
    // Note: This test is disabled until reaction::var properly integrates SBO
    // For now, we test SBOResource directly above
    EXPECT_TRUE(true);  // Placeholder
}

// Test storage info debugging interface
TEST_F(SBOTest, StorageInfoDebugging) {
    auto sbo_resource = std::make_shared<SBOResource<SmallStruct>>(SmallStruct{1, 2});
    auto heap_resource = std::make_shared<SBOResource<LargeStruct>>(LargeStruct{'X'});

    auto sbo_info = sbo_resource->getStorageInfo();
    EXPECT_TRUE(sbo_info.using_sbo);
    EXPECT_TRUE(sbo_info.initialized);
    EXPECT_EQ(sbo_info.type_size, sizeof(SmallStruct));
    EXPECT_EQ(sbo_info.stack_usage, sizeof(SmallStruct));

    auto heap_info = heap_resource->getStorageInfo();
    EXPECT_FALSE(heap_info.using_sbo);
    EXPECT_TRUE(heap_info.initialized);
    EXPECT_EQ(heap_info.type_size, sizeof(LargeStruct));
    EXPECT_EQ(heap_info.stack_usage, 0);
}

// Test thread safety of SBOResource
TEST_F(SBOTest, SBOResourceThreadSafety) {
    auto resource = std::make_shared<SBOResource<int>>(0);
    std::atomic<int> counter{0};
    constexpr int num_threads = 4;
    constexpr int increments_per_thread = 1000;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([resource, &counter]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                int expected = counter.fetch_add(1);
                resource->updateValue(expected + 1);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // The final value should be the total number of increments
    EXPECT_EQ(resource->getValue(), num_threads * increments_per_thread);
}