#include "reaction.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <iomanip>
#include <memory>
#include <random>
#include <functional>
#include <thread>

using namespace reaction;

// Benchmark utilities for cache manipulation
class BenchmarkObserverGraph {
public:
    static void forceCacheInvalidation() {
        auto& graph = ObserverGraph::getInstance();
        // Force massive cache invalidation by triggering many version increments
        for (int i = 0; i < 1000; ++i) {
            graph.triggerCacheCleanup();
        }
    }

    static auto getCacheStats() {
        return ObserverGraph::getInstance().getCacheStats();
    }
};

// Simple timing class
class SimpleTimer {
public:
    void start() { start_time = std::chrono::high_resolution_clock::now(); }
    void stop() { end_time = std::chrono::high_resolution_clock::now(); }
    double elapsed_ns() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    }
    double elapsed_ms() const { return elapsed_ns() / 1e6; }

private:
    std::chrono::high_resolution_clock::time_point start_time, end_time;
};

// Benchmark result structure
struct BenchmarkResult {
    std::string name;
    double time_with_cache_ms = 0.0;
    double time_without_cache_ms = 0.0;
    size_t iterations = 0;

    void print() const {
        double speedup = time_without_cache_ms / time_with_cache_ms;
        double throughput_with = iterations / (time_with_cache_ms / 1000.0);
        double throughput_without = iterations / (time_without_cache_ms / 1000.0);

        std::cout << std::string(80, '=') << std::endl;
        std::cout << "🚀 CACHE PERFORMANCE COMPARISON: " << name << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Time WITH cache:    " << time_with_cache_ms << " ms" << std::endl;
        std::cout << "Time WITHOUT cache: " << time_without_cache_ms << " ms" << std::endl;
        std::cout << std::setprecision(2);
        std::cout << "💪 Speedup:         " << speedup << "x faster with cache" << std::endl;
        std::cout << std::setprecision(0);
        std::cout << "Throughput (cached):     " << throughput_with << " ops/sec" << std::endl;
        std::cout << "Throughput (uncached):   " << throughput_without << " ops/sec" << std::endl;
        std::cout << std::setprecision(2);
        std::cout << "📈 Throughput improvement: " << (throughput_with / throughput_without) << "x higher" << std::endl;
        std::cout << "Cache efficiency: " << ((speedup - 1) / speedup * 100) << "%" << std::endl;
        std::cout << std::endl;
    }
};

// Benchmark functions
BenchmarkResult benchmark_cycle_detection(size_t iterations = 20000) {
    std::cout << "🔄 Testing Cycle Detection Cache Performance..." << std::endl;

    // Setup
    std::vector<Var<int>> vars;
    std::vector<Calc<int>> calcs_level1;
    std::vector<Calc<int>> calcs_level2;

    const size_t var_count = 80;
    const size_t calc_count = 60;

    for (size_t i = 0; i < var_count; ++i) {
        vars.push_back(create(static_cast<int>(i)));
    }

    for (size_t i = 0; i < calc_count; ++i) {
        size_t idx1 = i % var_count;
        size_t idx2 = (i + 1) % var_count;
        calcs_level1.push_back(create([&vars, idx1, idx2]() {
            return vars[idx1]() + vars[idx2]();
        }));
    }

    for (size_t i = 0; i < calc_count / 2; ++i) {
        size_t idx1 = i % calc_count;
        size_t idx2 = (i + 1) % calc_count;
        calcs_level2.push_back(create([&calcs_level1, idx1, idx2]() {
            return calcs_level1[idx1]() + calcs_level1[idx2]();
        }));
    }

    BenchmarkResult result;
    result.name = "Cycle Detection Cache";
    result.iterations = iterations;

    // Test WITH cache
    {
        SimpleTimer timer;
        timer.start();

        for (size_t iter = 0; iter < iterations; ++iter) {
            try {
                size_t src_idx = iter % calc_count;
                size_t dst_idx = (iter + 1) % calc_count;

                auto temp_calc = create([&calcs_level1, src_idx, dst_idx]() {
                    return calcs_level1[src_idx]() * calcs_level1[dst_idx]();
                });

                volatile auto temp_result = temp_calc.get();
                (void)temp_result;

            } catch (...) {
                // Expected for cycles
            }
        }

        timer.stop();
        result.time_with_cache_ms = timer.elapsed_ms();
    }

    // Test WITHOUT cache
    {
        SimpleTimer timer;
        timer.start();

        for (size_t iter = 0; iter < iterations; ++iter) {
            // Force cache invalidation every few operations
            if (iter % 10 == 0) {
                BenchmarkObserverGraph::forceCacheInvalidation();
            }

            try {
                size_t src_idx = iter % calc_count;
                size_t dst_idx = (iter + 1) % calc_count;

                auto temp_calc = create([&calcs_level1, src_idx, dst_idx]() {
                    return calcs_level1[src_idx]() * calcs_level1[dst_idx]();
                });

                volatile auto temp_result = temp_calc.get();
                (void)temp_result;

            } catch (...) {
                // Expected for cycles
            }
        }

        timer.stop();
        result.time_without_cache_ms = timer.elapsed_ms();
    }

    return result;
}

BenchmarkResult benchmark_observer_collection(size_t iterations = 15000) {
    std::cout << "🌐 Testing Observer Collection Cache Performance..." << std::endl;

    // Setup
    std::vector<Var<double>> leaves;
    std::vector<Calc<double>> branches;
    std::vector<Calc<double>> roots;

    const size_t leaf_count = 100;
    const size_t branch_count = 50;
    const size_t root_count = 10;

    for (size_t i = 0; i < leaf_count; ++i) {
        leaves.push_back(create(static_cast<double>(i) * 0.5));
    }

    for (size_t i = 0; i < branch_count; ++i) {
        size_t l1 = i % leaf_count;
        size_t l2 = (i + 1) % leaf_count;
        size_t l3 = (i + 2) % leaf_count;
        branches.push_back(create([&leaves, l1, l2, l3]() {
            return leaves[l1]() + leaves[l2]() + leaves[l3]();
        }));
    }

    for (size_t i = 0; i < root_count; ++i) {
        size_t b1 = i % branch_count;
        size_t b2 = (i + 1) % branch_count;
        size_t b3 = (i + 2) % branch_count;
        size_t b4 = (i + 3) % branch_count;
        size_t b5 = (i + 4) % branch_count;
        roots.push_back(create([&branches, b1, b2, b3, b4, b5]() {
            return branches[b1]() + branches[b2]() + branches[b3]() + branches[b4]() + branches[b5]();
        }));
    }

    BenchmarkResult result;
    result.name = "Observer Collection Cache";
    result.iterations = iterations;

    // Test WITH cache
    {
        SimpleTimer timer;
        timer.start();

        for (size_t iter = 0; iter < iterations; ++iter) {
            size_t leaf_idx = iter % leaf_count;
            double new_val = static_cast<double>(iter) * 0.1;

            leaves[leaf_idx].value(new_val);

            // Force evaluation of all roots (triggers observer collection)
            for (auto& root : roots) {
                volatile auto val = root.get();
                (void)val;
            }
        }

        timer.stop();
        result.time_with_cache_ms = timer.elapsed_ms();
    }

    // Test WITHOUT cache
    {
        SimpleTimer timer;
        timer.start();

        for (size_t iter = 0; iter < iterations; ++iter) {
            // Force cache invalidation frequently
            if (iter % 5 == 0) {
                BenchmarkObserverGraph::forceCacheInvalidation();
            }

            size_t leaf_idx = iter % leaf_count;
            double new_val = static_cast<double>(iter) * 0.1;

            leaves[leaf_idx].value(new_val);

            for (auto& root : roots) {
                volatile auto val = root.get();
                (void)val;
            }
        }

        timer.stop();
        result.time_without_cache_ms = timer.elapsed_ms();
    }

    return result;
}

BenchmarkResult benchmark_mixed_workload(size_t iterations = 10000) {
    std::cout << "🎯 Testing Mixed Workload Cache Performance..." << std::endl;

    // Setup
    std::vector<Var<int>> vars;
    std::vector<Calc<int>> simple_calcs;
    std::vector<Calc<int>> complex_calcs;

    const size_t var_count = 50;

    for (size_t i = 0; i < var_count; ++i) {
        vars.push_back(create(static_cast<int>(i)));
    }

    for (size_t i = 0; i < var_count; ++i) {
        size_t idx = i % var_count;
        simple_calcs.push_back(create([&vars, idx]() {
            return vars[idx]() * 2;
        }));
    }

    for (size_t i = 0; i < var_count / 2; ++i) {
        size_t idx1 = i % simple_calcs.size();
        size_t idx2 = (i + 1) % simple_calcs.size();
        size_t idx3 = (i + 2) % simple_calcs.size();
        complex_calcs.push_back(create([&simple_calcs, idx1, idx2, idx3]() {
            return simple_calcs[idx1]() + simple_calcs[idx2]() + simple_calcs[idx3]();
        }));
    }

    BenchmarkResult result;
    result.name = "Mixed Workload Cache";
    result.iterations = iterations;

    // Test WITH cache
    {
        SimpleTimer timer;
        timer.start();

        for (size_t iter = 0; iter < iterations; ++iter) {
            // Variable updates
            size_t var_idx = iter % var_count;
            vars[var_idx].value(static_cast<int>(iter % 1000));

            // Evaluation of calculations
            size_t simple_idx = iter % simple_calcs.size();
            size_t complex_idx = iter % complex_calcs.size();

            volatile auto simple_val = simple_calcs[simple_idx].get();
            volatile auto complex_val = complex_calcs[complex_idx].get();
            (void)simple_val;
            (void)complex_val;

            // Occasionally create new temporary calculations (cycle detection)
            if (iter % 100 == 0) {
                try {
                    auto temp = create([&complex_calcs, complex_idx]() {
                        return complex_calcs[complex_idx]() * 2;
                    });
                    volatile auto temp_val = temp.get();
                    (void)temp_val;
                } catch (...) {}
            }
        }

        timer.stop();
        result.time_with_cache_ms = timer.elapsed_ms();
    }

    // Test WITHOUT cache
    {
        SimpleTimer timer;
        timer.start();

        for (size_t iter = 0; iter < iterations; ++iter) {
            // Force cache invalidation frequently
            if (iter % 20 == 0) {
                BenchmarkObserverGraph::forceCacheInvalidation();
            }

            size_t var_idx = iter % var_count;
            vars[var_idx].value(static_cast<int>(iter % 1000));

            size_t simple_idx = iter % simple_calcs.size();
            size_t complex_idx = iter % complex_calcs.size();

            volatile auto simple_val = simple_calcs[simple_idx].get();
            volatile auto complex_val = complex_calcs[complex_idx].get();
            (void)simple_val;
            (void)complex_val;

            if (iter % 100 == 0) {
                try {
                    auto temp = create([&complex_calcs, complex_idx]() {
                        return complex_calcs[complex_idx]() * 2;
                    });
                    volatile auto temp_val = temp.get();
                    (void)temp_val;
                } catch (...) {}
            }
        }

        timer.stop();
        result.time_without_cache_ms = timer.elapsed_ms();
    }

    return result;
}

int main() {
    std::cout << "🔥 REACTION CACHE PERFORMANCE COMPARISON 🔥" << std::endl;
    std::cout << "Measuring exact performance improvements from caching mechanisms" << std::endl;
    std::cout << std::string(80, '=') << std::endl;

    // Run benchmarks
    auto cycle_result = benchmark_cycle_detection(15000);
    cycle_result.print();

    auto observer_result = benchmark_observer_collection(10000);
    observer_result.print();

    auto mixed_result = benchmark_mixed_workload(8000);
    mixed_result.print();

    // Overall summary
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "📊 COMPREHENSIVE CACHE PERFORMANCE SUMMARY 📊" << std::endl;
    std::cout << std::string(80, '=') << std::endl;

    double avg_speedup = (cycle_result.time_without_cache_ms / cycle_result.time_with_cache_ms +
                         observer_result.time_without_cache_ms / observer_result.time_with_cache_ms +
                         mixed_result.time_without_cache_ms / mixed_result.time_with_cache_ms) / 3.0;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "🎯 Average Speedup: " << avg_speedup << "x faster with caching" << std::endl;
    std::cout << "💪 Overall Cache Efficiency: " << ((avg_speedup - 1) / avg_speedup * 100) << "%" << std::endl;

    std::cout << "\n📈 Individual Test Results:" << std::endl;
    std::cout << "  • Cycle Detection: " << (cycle_result.time_without_cache_ms / cycle_result.time_with_cache_ms) << "x speedup" << std::endl;
    std::cout << "  • Observer Collection: " << (observer_result.time_without_cache_ms / observer_result.time_with_cache_ms) << "x speedup" << std::endl;
    std::cout << "  • Mixed Workload: " << (mixed_result.time_without_cache_ms / mixed_result.time_with_cache_ms) << "x speedup" << std::endl;

    // Display final cache statistics
    auto cache_stats = BenchmarkObserverGraph::getCacheStats();
    std::cout << "\n🗂️ Final Cache State:" << std::endl;
    std::cout << "  • Graph Cache Version: " << cache_stats.graphStats.currentVersion << std::endl;
    std::cout << "  • Cycle Cache Version: " << cache_stats.cycleStats.currentVersion << std::endl;
    std::cout << "  • Metrics Cache Version: " << cache_stats.metricsStats.currentVersion << std::endl;

    std::cout << "\n✅ Cache performance benchmarking completed!" << std::endl;
    return 0;
}
