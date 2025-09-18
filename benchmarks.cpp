/*
 * Performance Benchmarks for Reaction Framework
 * 
 * These benchmarks measure performance characteristics and help
 * identify performance regressions and optimization opportunities.
 */

#include "reaction/reaction.h"
#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>

using namespace reaction;

// Benchmark node creation and destruction
static void BM_NodeCreation(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<Var<int>> nodes;
        nodes.reserve(state.range(0));
        
        for (int i = 0; i < state.range(0); ++i) {
            nodes.push_back(var(i));
        }
        
        benchmark::DoNotOptimize(nodes);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_NodeCreation)->Range(8, 8<<10)->Complexity();

// Benchmark dependency graph creation
static void BM_DependencyGraphCreation(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<Var<int>> sources;
        std::vector<Calc<int>> calculations;
        
        const int n = state.range(0);
        sources.reserve(n);
        calculations.reserve(n);
        
        // Create sources
        for (int i = 0; i < n; ++i) {
            sources.push_back(var(i));
        }
        
        // Create calculations with dependencies
        for (int i = 0; i < n; ++i) {
            if (i == 0) {
                calculations.push_back(calc([](int x) { return x * 2; }, sources[0]));
            } else {
                calculations.push_back(calc([](int a, int b) { 
                    return a + b; 
                }, sources[i], calculations[i-1]));
            }
        }
        
        benchmark::DoNotOptimize(calculations);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_DependencyGraphCreation)->Range(8, 8<<8)->Complexity();

// Benchmark single value updates
static void BM_SingleValueUpdate(benchmark::State& state) {
    auto source = var(0);
    std::vector<Calc<int>> observers;
    
    const int observer_count = state.range(0);
    observers.reserve(observer_count);
    
    for (int i = 0; i < observer_count; ++i) {
        observers.push_back(calc([i](int x) { return x + i; }, source));
    }
    
    int value = 0;
    for (auto _ : state) {
        source.value(++value);
        benchmark::DoNotOptimize(observers);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_SingleValueUpdate)->Range(8, 8<<10)->Complexity();

// Benchmark batch updates
static void BM_BatchUpdate(benchmark::State& state) {
    const int n = state.range(0);
    std::vector<Var<int>> sources;
    std::vector<Calc<int>> observers;
    
    sources.reserve(n);
    observers.reserve(n);
    
    for (int i = 0; i < n; ++i) {
        sources.push_back(var(i));
        observers.push_back(calc([](int x) { return x * 2; }, sources[i]));
    }
    
    for (auto _ : state) {
        batchExecute([&]() {
            for (int i = 0; i < n; ++i) {
                sources[i].value(sources[i].get() + 1);
            }
        });
        benchmark::DoNotOptimize(observers);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_BatchUpdate)->Range(8, 8<<10)->Complexity();

// Benchmark deep dependency chains
static void BM_DeepDependencyChain(benchmark::State& state) {
    const int depth = state.range(0);
    
    for (auto _ : state) {
        auto root = var(1);
        std::vector<Calc<int>> chain;
        chain.reserve(depth);
        
        chain.push_back(calc([](int x) { return x + 1; }, root));
        for (int i = 1; i < depth; ++i) {
            chain.push_back(calc([](int x) { return x + 1; }, chain[i-1]));
        }
        
        // Trigger propagation
        root.value(100);
        int result = chain[depth-1].get();
        benchmark::DoNotOptimize(result);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_DeepDependencyChain)->Range(8, 8<<8)->Complexity();

// Benchmark wide dependency trees
static void BM_WideDependencyTree(benchmark::State& state) {
    const int width = state.range(0);
    
    for (auto _ : state) {
        std::vector<Var<int>> leaves;
        leaves.reserve(width);
        
        for (int i = 0; i < width; ++i) {
            leaves.push_back(var(i));
        }
        
        auto aggregator = calc([&leaves]() {
            int sum = 0;
            for (const auto& leaf : leaves) {
                sum += leaf();
            }
            return sum;
        });
        
        // Trigger calculation
        int result = aggregator.get();
        benchmark::DoNotOptimize(result);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_WideDependencyTree)->Range(8, 8<<10)->Complexity();

// Benchmark expression evaluation
static void BM_ExpressionEvaluation(benchmark::State& state) {
    const int expr_count = state.range(0);
    std::vector<Var<double>> vars;
    vars.reserve(expr_count);
    
    for (int i = 0; i < expr_count; ++i) {
        vars.push_back(var(static_cast<double>(i + 1)));
    }
    
    // Create complex expression
    auto complex_expr = expr(vars[0] + vars[1]);
    for (int i = 2; i < expr_count; ++i) {
        complex_expr = expr(complex_expr + vars[i]);
    }
    
    for (auto _ : state) {
        double result = complex_expr.get();
        benchmark::DoNotOptimize(result);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ExpressionEvaluation)->Range(8, 8<<8)->Complexity();

// Benchmark field-based objects
static void BM_FieldBasedObjects(benchmark::State& state) {
    class TestObject : public FieldBase {
    public:
        TestObject(int id) : 
            m_id(field(id)),
            m_value(field(id * 10)) {}
        
        void updateValue(int new_value) { m_value.value(new_value); }
        int getValue() const { return m_value.get(); }
        
    private:
        Var<int> m_id;
        Var<int> m_value;
    };
    
    const int object_count = state.range(0);
    std::vector<Var<TestObject>> objects;
    std::vector<Calc<int>> observers;
    
    objects.reserve(object_count);
    observers.reserve(object_count);
    
    for (int i = 0; i < object_count; ++i) {
        objects.push_back(var(TestObject(i)));
        observers.push_back(calc([](const TestObject& obj) {
            return obj.getValue() * 2;
        }, objects[i]));
    }
    
    for (auto _ : state) {
        for (int i = 0; i < object_count; ++i) {
            objects[i]->updateValue(i * 100);
        }
        benchmark::DoNotOptimize(observers);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_FieldBasedObjects)->Range(8, 8<<8)->Complexity();

// Benchmark memory usage patterns
static void BM_MemoryUsage(benchmark::State& state) {
    const int iterations = state.range(0);
    
    for (auto _ : state) {
        std::vector<Var<std::string>> temp_vars;
        std::vector<Calc<std::string>> temp_calcs;
        
        temp_vars.reserve(iterations);
        temp_calcs.reserve(iterations);
        
        for (int i = 0; i < iterations; ++i) {
            temp_vars.push_back(var(std::string("temp_") + std::to_string(i)));
            temp_calcs.push_back(calc([](const std::string& s) {
                return s + "_processed";
            }, temp_vars[i]));
        }
        
        // Use the calculations
        for (int i = 0; i < iterations; ++i) {
            std::string result = temp_calcs[i].get();
            benchmark::DoNotOptimize(result);
        }
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_MemoryUsage)->Range(8, 8<<8)->Complexity();

// Benchmark concurrent operations
static void BM_ConcurrentOperations(benchmark::State& state) {
    const int thread_count = std::min(static_cast<int>(std::thread::hardware_concurrency()), 8);
    std::vector<Var<int>> vars(thread_count);
    
    for (int i = 0; i < thread_count; ++i) {
        vars[i] = var(i);
    }
    
    for (auto _ : state) {
        std::vector<std::future<void>> futures;
        
        for (int t = 0; t < thread_count; ++t) {
            futures.push_back(std::async(std::launch::async, [&vars, t]() {
                for (int i = 0; i < 100; ++i) {
                    vars[t].value(vars[t].get() + 1);
                }
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
    }
    state.SetItemsProcessed(state.iterations() * thread_count * 100);
}
BENCHMARK(BM_ConcurrentOperations)->UseRealTime();

// Benchmark trigger mode performance
static void BM_TriggerModes(benchmark::State& state) {
    auto source = var(0);
    
    std::atomic<int> always_count{0};
    std::atomic<int> change_count{0};
    
    auto always_action = action<AlwaysTrig>([&always_count](int) {
        always_count++;
    }, source);
    
    auto change_action = action<ChangeTrig>([&change_count](int) {
        change_count++;
    }, source);
    
    for (auto _ : state) {
        source.value(source.get()); // Same value
        source.value(source.get() + 1); // Different value
    }
    
    state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_TriggerModes);

// Benchmark complex data structures
static void BM_ComplexDataStructures(benchmark::State& state) {
    using ComplexType = std::map<std::string, std::vector<std::pair<int, double>>>;
    
    ComplexType complex_data;
    for (int i = 0; i < 100; ++i) {
        std::string key = "key_" + std::to_string(i);
        std::vector<std::pair<int, double>> values;
        for (int j = 0; j < 10; ++j) {
            values.emplace_back(j, static_cast<double>(j) * 1.5);
        }
        complex_data[key] = std::move(values);
    }
    
    auto complex_var = var(complex_data);
    auto complex_calc = calc([](const ComplexType& data) {
        double sum = 0.0;
        for (const auto& [key, values] : data) {
            for (const auto& [i, d] : values) {
                sum += static_cast<double>(i) + d;
            }
        }
        return sum;
    }, complex_var);
    
    for (auto _ : state) {
        double result = complex_calc.get();
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ComplexDataStructures);

// Custom main function to run benchmarks
BENCHMARK_MAIN();