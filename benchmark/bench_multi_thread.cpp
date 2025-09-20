#include "reaction.h"
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
 * @brief 优化的多线程场景下reaction框架吞吐量测试
 *
 * 本测试解决了原始版本的问题：
 * 1. 减少线程间竞争热点
 * 2. 更真实地模拟实际应用场景
 * 3. 分离读写测试，避免混合操作的干扰
 */

// ============================================================================
// 独立数据源并发读取测试（无竞争）
// ============================================================================

static void BM_MultiThread_IndependentRead(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int operations_per_thread = 1000;

    // 每个线程有独立的数据源和计算链，避免竞争
    std::vector<std::vector<Calc<int>>> thread_calcs(num_threads);
    std::vector<Var<int>> thread_sources;

    for (int t = 0; t < num_threads; ++t) {
        thread_sources.emplace_back(var(t));

        // 每个线程创建独立的计算链
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
                    // 每个线程只读取自己的计算链，无竞争
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
// 独立数据源并发写入测试（无竞争）
// ============================================================================

static void BM_MultiThread_IndependentWrite(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int operations_per_thread = 100;

    // 每个线程有独立的数据源和计算节点
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
                    // 每个线程只修改自己的数据源，无竞争
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
// 生产者-消费者模式测试
// ============================================================================

static void BM_MultiThread_ProducerConsumer(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int operations_per_thread = 100;

    // 生产者线程数 = 线程总数 / 2，消费者线程数 = 线程总数 / 2
    const int num_producers = std::max(1, num_threads / 2);
    const int num_consumers = num_threads - num_producers;

    // 创建多个数据源供生产者写入
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

        // 生产者线程
        for (int p = 0; p < num_producers; ++p) {
            threads.emplace_back([&, p]() {
                sync_point.arrive_and_wait();

                for (int op = 0; op < operations_per_thread; ++op) {
                    data_sources[p].value(data_sources[p].get() + 1);
                }

                total_operations += operations_per_thread;
            });
        }

        // 消费者线程
        for (int c = 0; c < num_consumers; ++c) {
            threads.emplace_back([&, c]() {
                sync_point.arrive_and_wait();

                for (int op = 0; op < operations_per_thread; ++op) {
                    // 消费者轮询读取不同的数据源
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
// 基准测试注册
// ============================================================================

// 独立数据源测试（展示最佳性能）
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

// 生产者-消费者模式测试
BENCHMARK(BM_MultiThread_ProducerConsumer)->Arg(2)->UseRealTime();
BENCHMARK(BM_MultiThread_ProducerConsumer)->Arg(4)->UseRealTime();
BENCHMARK(BM_MultiThread_ProducerConsumer)->Arg(8)->UseRealTime();
BENCHMARK(BM_MultiThread_ProducerConsumer)->Arg(16)->UseRealTime();
BENCHMARK(BM_MultiThread_ProducerConsumer)->Arg(32)->UseRealTime();

BENCHMARK_MAIN();
