/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

/**
 * @file sbo_example.cpp
 * @brief Example demonstrating Small Buffer Optimization (SBO) in Reaction framework
 *
 * This example shows how to enable SBO and demonstrates the memory usage differences
 * between standard and optimized resource implementations.
 */

// Enable SBO before including reaction headers
#define REACTION_ENABLE_SBO 1

#include <iostream>
#include <chrono>
#include <vector>
#include <string>

#include "../include/reaction/reaction.h"

using namespace reaction;

/**
 * @brief Small struct that fits in SBO threshold
 */
struct SmallData {
    int x, y;
    float z;
    
    SmallData(int x = 0, int y = 0, float z = 0.0f) : x(x), y(y), z(z) {}
    
    bool operator==(const SmallData& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
    
    bool operator!=(const SmallData& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Large struct that exceeds SBO threshold
 */
struct LargeData {
    std::array<double, 10> data;
    std::string description;
    
    LargeData() : data{}, description("large data") {}
    
    bool operator==(const LargeData& other) const {
        return data == other.data && description == other.description;
    }
    
    bool operator!=(const LargeData& other) const {
        return !(*this == other);
    }
};

void demonstrateSBOInfo() {
    std::cout << "=== Small Buffer Optimization Information ===\n";
    std::cout << "SBO Enabled: " << (ResourceInfo::isSBOEnabled() ? "Yes" : "No") << "\n";
    std::cout << "SBO Threshold: " << SBO_THRESHOLD << " bytes\n\n";
    
    // Show storage strategies for different types
    std::cout << "Storage strategies:\n";
    std::cout << "int: " << ResourceInfo::getStorageStrategy<int>() 
              << " (size: " << sizeof(int) << " bytes, storage: " 
              << ResourceInfo::getStorageSize<int>() << " bytes)\n";
    
    std::cout << "double: " << ResourceInfo::getStorageStrategy<double>() 
              << " (size: " << sizeof(double) << " bytes, storage: " 
              << ResourceInfo::getStorageSize<double>() << " bytes)\n";
    
    std::cout << "SmallData: " << ResourceInfo::getStorageStrategy<SmallData>() 
              << " (size: " << sizeof(SmallData) << " bytes, storage: " 
              << ResourceInfo::getStorageSize<SmallData>() << " bytes)\n";
    
    std::cout << "LargeData: " << ResourceInfo::getStorageStrategy<LargeData>() 
              << " (size: " << sizeof(LargeData) << " bytes, storage: " 
              << ResourceInfo::getStorageSize<LargeData>() << " bytes)\n";
    
    std::cout << "std::string: " << ResourceInfo::getStorageStrategy<std::string>() 
              << " (size: " << sizeof(std::string) << " bytes, storage: " 
              << ResourceInfo::getStorageSize<std::string>() << " bytes)\n\n";
}

void demonstrateBasicUsage() {
    std::cout << "=== Basic SBO Usage ===\n";
    
    // Small types use SBO (stack allocation)
    auto int_var = var(42);
    auto double_var = var(3.14);
    auto small_var = var(SmallData{1, 2, 3.5f});
    
    // Large types still use heap allocation
    auto large_var = var(LargeData{});
    auto string_var = var(std::string("Hello, SBO!"));
    
    std::cout << "Created reactive variables:\n";
    std::cout << "int_var: " << int_var.get() << " (SBO: " << decltype(int_var)::value_type() << ")\n";
    std::cout << "double_var: " << double_var.get() << "\n";
    std::cout << "small_var: x=" << small_var.get().x << ", y=" << small_var.get().y << ", z=" << small_var.get().z << "\n";
    std::cout << "string_var: " << string_var.get() << "\n\n";
    
    // Demonstrate reactive calculations with SBO
    auto sum = calc([](int a, double b) { return a + b; }, int_var, double_var);
    auto product = calc([](const SmallData& s) { return s.x * s.y * s.z; }, small_var);
    
    std::cout << "Reactive calculations:\n";
    std::cout << "sum: " << sum.get() << "\n";
    std::cout << "product: " << product.get() << "\n\n";
    
    // Update values and see reactive updates
    int_var.value(100);
    small_var.value(SmallData{5, 6, 2.0f});
    
    std::cout << "After updates:\n";
    std::cout << "sum: " << sum.get() << "\n";
    std::cout << "product: " << product.get() << "\n\n";
}

void performanceComparison() {
    std::cout << "=== Performance Comparison ===\n";
    
    const int iterations = 100000;
    
    // Benchmark small data creation and updates
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<decltype(var(0))> int_vars;
    int_vars.reserve(iterations);
    
    // Create many small reactive variables
    for (int i = 0; i < iterations; ++i) {
        int_vars.push_back(var(i));
    }
    
    auto creation_end = std::chrono::high_resolution_clock::now();
    
    // Update all variables
    for (int i = 0; i < iterations; ++i) {
        int_vars[i].value(i * 2);
    }
    
    auto update_end = std::chrono::high_resolution_clock::now();
    
    auto creation_time = std::chrono::duration_cast<std::chrono::microseconds>(creation_end - start);
    auto update_time = std::chrono::duration_cast<std::chrono::microseconds>(update_end - creation_end);
    
    std::cout << "Created " << iterations << " int reactive variables in: " 
              << creation_time.count() << " μs\n";
    std::cout << "Updated " << iterations << " int reactive variables in: " 
              << update_time.count() << " μs\n";
    
    std::cout << "Average creation time per variable: " 
              << static_cast<double>(creation_time.count()) / iterations << " μs\n";
    std::cout << "Average update time per variable: " 
              << static_cast<double>(update_time.count()) / iterations << " μs\n\n";
}

void demonstrateMemoryFootprint() {
    std::cout << "=== Memory Footprint Analysis ===\n";
    
    std::cout << "Object sizes:\n";
    std::cout << "sizeof(int): " << sizeof(int) << " bytes\n";
    std::cout << "sizeof(std::unique_ptr<int>): " << sizeof(std::unique_ptr<int>) << " bytes\n";
    std::cout << "sizeof(OptimizedResource<int>): " << sizeof(OptimizedResource<int>) << " bytes\n";
    std::cout << "sizeof(Resource<int>): " << sizeof(Resource<int>) << " bytes\n\n";
    
    std::cout << "For small types like int, SBO can reduce memory overhead and improve cache locality.\n";
    std::cout << "SBO eliminates one level of indirection and heap allocation.\n\n";
}

int main() {
    try {
        std::cout << "Reaction Framework - Small Buffer Optimization Example\n";
        std::cout << "=====================================================\n\n";
        
        demonstrateSBOInfo();
        demonstrateBasicUsage();
        performanceComparison();
        demonstrateMemoryFootprint();
        
        std::cout << "Example completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}