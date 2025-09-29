/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

/**
 * @file bench_sbo.cpp
 * @brief Benchmark comparing standard vs SBO-optimized resource implementations
 *
 * This benchmark measures the performance difference between heap-allocated
 * and stack-allocated (SBO) resource storage for small objects.
 */

#include <benchmark/benchmark.h>
#include <vector>
#include <memory>
#include <random>

// Test both implementations
#define REACTION_ENABLE_SBO 0
#include "../include/reaction/reaction.h"

#undef REACTION_ENABLE_SBO
#define REACTION_ENABLE_SBO 1
#include "../include/reaction/core/resource_optimized.h"

using namespace reaction;

// Test data structures
struct SmallPOD {
    int x, y;
    double z;
    
    SmallPOD(int x = 0, int y = 0, double z = 0.0) : x(x), y(y), z(z) {}
    
    bool operator==(const SmallPOD& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
    
    bool operator!=(const SmallPOD& other) const {
        return !(*this == other);
    }
};

// Benchmark: Creating many small reactive variables
static void BM_CreateSmallVars_Standard(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<Resource<int>> vars;
        vars.reserve(state.range(0));
        
        for (int i = 0; i < state.range(0); ++i) {
            vars.emplace_back(i);
        }
        
        benchmark::DoNotOptimize(vars);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

static void BM_CreateSmallVars_SBO(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<OptimizedResource<int>> vars;
        vars.reserve(state.range(0));
        
        for (int i = 0; i < state.range(0); ++i) {
            vars.emplace_back(i);
        }
        
        benchmark::DoNotOptimize(vars);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// Benchmark: Updating values
static void BM_UpdateValues_Standard(benchmark::State& state) {
    std::vector<Resource<int>> vars;
    vars.reserve(state.range(0));
    
    for (int i = 0; i < state.range(0); ++i) {
        vars.emplace_back(i);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);
    
    for (auto _ : state) {
        for (auto& var : vars) {
            var.updateValue(dis(gen));
        }
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

static void BM_UpdateValues_SBO(benchmark::State& state) {
    std::vector<OptimizedResource<int>> vars;
    vars.reserve(state.range(0));
    
    for (int i = 0; i < state.range(0); ++i) {
        vars.emplace_back(i);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);
    
    for (auto _ : state) {
        for (auto& var : vars) {
            var.updateValue(dis(gen));
        }
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// Benchmark: Reading values
static void BM_ReadValues_Standard(benchmark::State& state) {
    std::vector<Resource<int>> vars;
    vars.reserve(state.range(0));
    
    for (int i = 0; i < state.range(0); ++i) {
        vars.emplace_back(i);
    }
    
    for (auto _ : state) {
        int sum = 0;
        for (const auto& var : vars) {
            sum += var.getValue();
        }
        benchmark::DoNotOptimize(sum);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

static void BM_ReadValues_SBO(benchmark::State& state) {
    std::vector<OptimizedResource<int>> vars;
    vars.reserve(state.range(0));
    
    for (int i = 0; i < state.range(0); ++i) {
        vars.emplace_back(i);
    }
    
    for (auto _ : state) {
        int sum = 0;
        for (const auto& var : vars) {
            sum += var.getValue();
        }
        benchmark::DoNotOptimize(sum);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// Benchmark: Small POD structures
static void BM_SmallPOD_Standard(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<Resource<SmallPOD>> vars;
        vars.reserve(state.range(0));
        
        for (int i = 0; i < state.range(0); ++i) {
            vars.emplace_back(SmallPOD{i, i*2, i*3.14});
        }
        
        // Update some values
        for (int i = 0; i < state.range(0) / 2; ++i) {
            vars[i].updateValue(SmallPOD{i+1000, i+2000, i+3000.0});
        }
        
        benchmark::DoNotOptimize(vars);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

static void BM_SmallPOD_SBO(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<OptimizedResource<SmallPOD>> vars;
        vars.reserve(state.range(0));
        
        for (int i = 0; i < state.range(0); ++i) {
            vars.emplace_back(SmallPOD{i, i*2, i*3.14});
        }
        
        // Update some values
        for (int i = 0; i < state.range(0) / 2; ++i) {
            vars[i].updateValue(SmallPOD{i+1000, i+2000, i+3000.0});
        }
        
        benchmark::DoNotOptimize(vars);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// Register benchmarks with different sizes
BENCHMARK(BM_CreateSmallVars_Standard)->Range(100, 10000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_CreateSmallVars_SBO)->Range(100, 10000)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_UpdateValues_Standard)->Range(100, 10000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_UpdateValues_SBO)->Range(100, 10000)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_ReadValues_Standard)->Range(100, 10000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_ReadValues_SBO)->Range(100, 10000)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_SmallPOD_Standard)->Range(100, 5000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_SmallPOD_SBO)->Range(100, 5000)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();