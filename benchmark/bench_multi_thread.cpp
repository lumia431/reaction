#include "reaction/reaction.h"
#include <benchmark/benchmark.h>
#include <thread>
#include <vector>
#include <atomic>
#include <barrier>
#include <chrono>
#include <random>
#include <functional>

using namespace reaction;

/**
 * @brief Optimized throughput benchmark for the Reaction framework in multithreaded scenarios
 *
 * This benchmark addresses issues in the original version:
 * 1. Reduces contention between threads
 * 2. Simulates more realistic application scenarios
 * 3. Separates read and write tests to avoid interference
 */

// ============================================================================
// Independent data source concurrent read test (no contention)
// ============================================================================

static void BM_MultiThread_IndependentRead(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int operations_per_thread = 1000;

    // Each thread has its own data source and computation chain to avoid contention
    std::vector<std::vector<Calc<int>>> thread_calcs(num_threads);
    std::vector<Var<int>> thread_sources;

    for (int t = 0; t < num_threads; ++t) {
        thread_sources.emplace_back(var(t));

        // Each thread creates its own independent computation chain
        for (int i = 0; i < 10; ++i) {
            if (i == 0) {
                thread_calcs[t].emplace_back(calc([&thread_sources, t]() {
                    return thread_sources[t]() + 1;
                }));
            } else {
                const int prev_idx = i - 1;
                thread_calcs[t].emplace_back(calc([&thread_calcs, t, prev_idx]() {
                    return thread_calcs[t][prev_idx]() + 1;
                }));
            }
        }
    }

    std::atomic<long long> total_operations{0};

    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::barrier sync_point(num_threads + 1);

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                sync_point.arrive_and_wait();

                for (int op = 0; op < operations_per_thread; ++op) {
                    // Each thread only reads its own computation chain, no contention
                    int result = thread_calcs[t].back().get();
                    benchmark::DoNotOptimize(result);
                }

                total_operations += operations_per_thread;
            });
        }

        sync_point.arrive_and_wait();

        for (auto& thread : threads) {
            thread.join();
        }
    }

    state.SetItemsProcessed(total_operations.load());
    state.counters["ThreadCount"] = num_threads;
    state.counters["OpsPerSecond"] = benchmark::Counter(
        total_operations.load(), benchmark::Counter::kIsRate);
}

// ============================================================================
// Independent data source concurrent write test (no contention)
// ============================================================================

static void BM_MultiThread_IndependentWrite(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int operations_per_thread = 100;

    // Each thread has its own data source and computation node
    std::vector<Var<int>> thread_sources;
    std::vector<Calc<int>> thread_calcs;

    for (int t = 0; t < num_threads; ++t) {
        thread_sources.emplace_back(var(t));
        thread_calcs.emplace_back(calc([&thread_sources, t]() {
            return thread_sources[t]() * 2;
        }));
    }

    std::atomic<long long> total_operations{0};

    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::barrier sync_point(num_threads + 1);

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                sync_point.arrive_and_wait();

                for (int op = 0; op < operations_per_thread; ++op) {
                    // Each thread only updates its own data source, no contention
                    thread_sources[t].value(thread_sources[t].get() + 1);
                    int result = thread_calcs[t].get();
                    benchmark::DoNotOptimize(result);
                }

                total_operations += operations_per_thread;
            });
        }

        sync_point.arrive_and_wait();

        for (auto& thread : threads) {
            thread.join();
        }
    }

    state.SetItemsProcessed(total_operations.load());
    state.counters["ThreadCount"] = num_threads;
    state.counters["OpsPerSecond"] = benchmark::Counter(
        total_operations.load(), benchmark::Counter::kIsRate);
}

// ============================================================================
// Producer-consumer pattern test
// ============================================================================

static void BM_MultiThread_ProducerConsumer(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int operations_per_thread = 100;

    // Number of producer threads = num_threads / 2, consumers = the rest
    const int num_producers = std::max(1, num_threads / 2);
    const int num_consumers = num_threads - num_producers;

    // Create multiple data sources for producers
    std::vector<Var<int>> data_sources;
    std::vector<Calc<int>> data_calcs;

    for (int i = 0; i < num_producers; ++i) {
        data_sources.emplace_back(var(i));
        data_calcs.emplace_back(calc([&data_sources, i]() {
            return data_sources[i]() + 100;
        }));
    }

    std::atomic<long long> total_operations{0};

    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::barrier sync_point(num_threads + 1);

        // Producer threads
        for (int p = 0; p < num_producers; ++p) {
            threads.emplace_back([&, p]() {
                sync_point.arrive_and_wait();

                for (int op = 0; op < operations_per_thread; ++op) {
                    data_sources[p].value(data_sources[p].get() + 1);
                }

                total_operations += operations_per_thread;
            });
        }

        // Consumer threads
        for (int c = 0; c < num_consumers; ++c) {
            threads.emplace_back([&, c]() {
                sync_point.arrive_and_wait();

                for (int op = 0; op < operations_per_thread; ++op) {
                    // Consumers poll different data sources
                    int source_idx = (c + op) % data_calcs.size();
                    int result = data_calcs[source_idx].get();
                    benchmark::DoNotOptimize(result);
                }

                total_operations += operations_per_thread;
            });
        }

        sync_point.arrive_and_wait();

        for (auto& thread : threads) {
            thread.join();
        }
    }

    state.SetItemsProcessed(total_operations.load());
    state.counters["ThreadCount"] = num_threads;
    state.counters["Producers"] = num_producers;
    state.counters["Consumers"] = num_consumers;
    state.counters["OpsPerSecond"] = benchmark::Counter(
        total_operations.load(), benchmark::Counter::kIsRate);
}

// ============================================================================
// Benchmark registration
// ============================================================================

// Independent data source test (shows best-case performance)
BENCHMARK(BM_MultiThread_IndependentRead)->Arg(1)->UseRealTime();
BENCHMARK(BM_MultiThread_IndependentRead)->Arg(2)->UseRealTime();
BENCHMARK(BM_MultiThread_IndependentRead)->Arg(4)->UseRealTime();
BENCHMARK(BM_MultiThread_IndependentRead)->Arg(8)->UseRealTime();
BENCHMARK(BM_MultiThread_IndependentRead)->Arg(16)->UseRealTime();
BENCHMARK(BM_MultiThread_IndependentRead)->Arg(32)->UseRealTime();

BENCHMARK(BM_MultiThread_IndependentWrite)->Arg(1)->UseRealTime();
BENCHMARK(BM_MultiThread_IndependentWrite)->Arg(2)->UseRealTime();
BENCHMARK(BM_MultiThread_IndependentWrite)->Arg(4)->UseRealTime();
BENCHMARK(BM_MultiThread_IndependentWrite)->Arg(8)->UseRealTime();
BENCHMARK(BM_MultiThread_IndependentWrite)->Arg(16)->UseRealTime();
BENCHMARK(BM_MultiThread_IndependentWrite)->Arg(32)->UseRealTime();

// Producer-consumer pattern test
BENCHMARK(BM_MultiThread_ProducerConsumer)->Arg(2)->UseRealTime();
BENCHMARK(BM_MultiThread_ProducerConsumer)->Arg(4)->UseRealTime();
BENCHMARK(BM_MultiThread_ProducerConsumer)->Arg(8)->UseRealTime();
BENCHMARK(BM_MultiThread_ProducerConsumer)->Arg(16)->UseRealTime();
BENCHMARK(BM_MultiThread_ProducerConsumer)->Arg(32)->UseRealTime();

BENCHMARK_MAIN();
