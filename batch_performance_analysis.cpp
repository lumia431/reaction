/*
 * Batch Performance Analysis for Reaction Framework
 * 
 * 分析批处理在什么情况下性能提升不明显，以及如何改进
 */

#include "reaction/reaction.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>

using namespace reaction;
using namespace std::chrono;

// 性能测试辅助类
class PerformanceTimer {
public:
    void start() { m_start = high_resolution_clock::now(); }
    
    long long stop_microseconds() {
        auto end = high_resolution_clock::now();
        return duration_cast<microseconds>(end - m_start).count();
    }
    
    void print_result(const std::string& test_name, long long microseconds) {
        std::cout << test_name << ": " << microseconds << " μs" << std::endl;
    }
    
private:
    high_resolution_clock::time_point m_start;
};

void test_batch_performance_scenarios() {
    std::cout << "=== Batch Performance Analysis ===" << std::endl;
    
    PerformanceTimer timer;
    
    // 场景1: 单一依赖链 - 批处理效果不明显
    {
        std::cout << "\n1. Single Dependency Chain (批处理效果有限):" << std::endl;
        
        auto root = var(1);
        std::vector<Calc<int>> chain;
        const int CHAIN_LENGTH = 100;
        
        // 创建线性依赖链: root -> calc1 -> calc2 -> ... -> calc100
        chain.push_back(calc([](int x) { return x + 1; }, root));
        for (int i = 1; i < CHAIN_LENGTH; ++i) {
            chain.push_back(calc([](int x) { return x + 1; }, chain[i-1]));
        }
        
        // 测试无批处理
        timer.start();
        for (int i = 0; i < 1000; ++i) {
            root.value(i);
            volatile int result = chain.back().get(); // 防止优化
            (void)result;
        }
        auto no_batch_time = timer.stop_microseconds();
        timer.print_result("No batch (single chain)", no_batch_time);
        
        // 测试批处理
        timer.start();
        for (int i = 0; i < 1000; ++i) {
            batchExecute([&]() {
                root.value(i);
            });
            volatile int result = chain.back().get();
            (void)result;
        }
        auto batch_time = timer.stop_microseconds();
        timer.print_result("With batch (single chain)", batch_time);
        
        double improvement = (double)no_batch_time / batch_time;
        std::cout << "Performance improvement: " << improvement << "x" << std::endl;
        
        if (improvement < 1.2) {
            std::cout << "❌ 批处理效果不明显！原因：线性依赖链中每个节点都必须更新" << std::endl;
        }
    }
    
    // 场景2: 独立节点 - 批处理效果明显
    {
        std::cout << "\n2. Independent Nodes (批处理效果显著):" << std::endl;
        
        const int NODE_COUNT = 100;
        std::vector<Var<int>> sources;
        std::vector<Calc<int>> observers;
        
        // 创建独立的源和观察者
        for (int i = 0; i < NODE_COUNT; ++i) {
            sources.push_back(var(i));
            observers.push_back(calc([](int x) { return x * 2; }, sources[i]));
        }
        
        // 测试无批处理
        timer.start();
        for (int iter = 0; iter < 100; ++iter) {
            for (int i = 0; i < NODE_COUNT; ++i) {
                sources[i].value(iter * NODE_COUNT + i);
            }
            // 读取所有结果
            for (int i = 0; i < NODE_COUNT; ++i) {
                volatile int result = observers[i].get();
                (void)result;
            }
        }
        auto no_batch_time = timer.stop_microseconds();
        timer.print_result("No batch (independent)", no_batch_time);
        
        // 测试批处理
        timer.start();
        for (int iter = 0; iter < 100; ++iter) {
            batchExecute([&]() {
                for (int i = 0; i < NODE_COUNT; ++i) {
                    sources[i].value(iter * NODE_COUNT + i);
                }
            });
            for (int i = 0; i < NODE_COUNT; ++i) {
                volatile int result = observers[i].get();
                (void)result;
            }
        }
        auto batch_time = timer.stop_microseconds();
        timer.print_result("With batch (independent)", batch_time);
        
        double improvement = (double)no_batch_time / batch_time;
        std::cout << "Performance improvement: " << improvement << "x" << std::endl;
        
        if (improvement > 1.5) {
            std::cout << "✅ 批处理效果显著！原因：减少了中间通知" << std::endl;
        }
    }
    
    // 场景3: 重复依赖 - 批处理最有效
    {
        std::cout << "\n3. Repeated Dependencies (批处理最有效):" << std::endl;
        
        auto shared_source = var(1);
        const int OBSERVER_COUNT = 100;
        std::vector<Calc<int>> observers;
        
        // 多个观察者依赖同一个源
        for (int i = 0; i < OBSERVER_COUNT; ++i) {
            observers.push_back(calc([i](int x) { return x + i; }, shared_source));
        }
        
        // 测试无批处理
        timer.start();
        for (int i = 0; i < 1000; ++i) {
            shared_source.value(i);
            // 读取所有观察者
            for (auto& observer : observers) {
                volatile int result = observer.get();
                (void)result;
            }
        }
        auto no_batch_time = timer.stop_microseconds();
        timer.print_result("No batch (repeated deps)", no_batch_time);
        
        // 测试批处理
        timer.start();
        for (int i = 0; i < 1000; ++i) {
            batchExecute([&]() {
                shared_source.value(i);
            });
            for (auto& observer : observers) {
                volatile int result = observer.get();
                (void)result;
            }
        }
        auto batch_time = timer.stop_microseconds();
        timer.print_result("With batch (repeated deps)", batch_time);
        
        double improvement = (double)no_batch_time / batch_time;
        std::cout << "Performance improvement: " << improvement << "x" << std::endl;
        
        if (improvement > 2.0) {
            std::cout << "✅ 批处理效果最佳！原因：避免了重复通知同一组观察者" << std::endl;
        }
    }
    
    // 场景4: 复杂依赖图 - 分析瓶颈
    {
        std::cout << "\n4. Complex Dependency Graph:" << std::endl;
        
        const int LAYERS = 10;
        const int NODES_PER_LAYER = 20;
        
        std::vector<std::vector<Var<int>>> layers(LAYERS);
        std::vector<std::vector<Calc<int>>> calc_layers(LAYERS - 1);
        
        // 创建第一层（源节点）
        for (int i = 0; i < NODES_PER_LAYER; ++i) {
            layers[0].push_back(var(i));
        }
        
        // 创建后续层（每个节点依赖前一层的多个节点）
        for (int layer = 1; layer < LAYERS; ++layer) {
            for (int i = 0; i < NODES_PER_LAYER; ++i) {
                layers[layer].push_back(var(0)); // 占位符
            }
            
            for (int i = 0; i < NODES_PER_LAYER; ++i) {
                if (layer == 1) {
                    // 第二层直接依赖第一层
                    int dep1 = i % NODES_PER_LAYER;
                    int dep2 = (i + 1) % NODES_PER_LAYER;
                    calc_layers[layer-1].push_back(calc([](int a, int b) { 
                        return (a + b) % 1000; 
                    }, layers[0][dep1], layers[0][dep2]));
                } else {
                    // 后续层依赖前一层的计算结果
                    int dep1 = i % NODES_PER_LAYER;
                    int dep2 = (i + 1) % NODES_PER_LAYER;
                    calc_layers[layer-1].push_back(calc([](int a, int b) { 
                        return (a + b) % 1000; 
                    }, calc_layers[layer-2][dep1], calc_layers[layer-2][dep2]));
                }
            }
        }
        
        // 测试批量更新第一层
        timer.start();
        for (int iter = 0; iter < 100; ++iter) {
            // 无批处理：逐个更新
            for (int i = 0; i < NODES_PER_LAYER; ++i) {
                layers[0][i].value(iter * NODES_PER_LAYER + i);
            }
            // 读取最后一层的结果
            for (int i = 0; i < NODES_PER_LAYER; ++i) {
                volatile int result = calc_layers.back()[i].get();
                (void)result;
            }
        }
        auto no_batch_time = timer.stop_microseconds();
        timer.print_result("No batch (complex graph)", no_batch_time);
        
        timer.start();
        for (int iter = 0; iter < 100; ++iter) {
            // 批处理：一次性更新所有源
            batchExecute([&]() {
                for (int i = 0; i < NODES_PER_LAYER; ++i) {
                    layers[0][i].value(iter * NODES_PER_LAYER + i);
                }
            });
            for (int i = 0; i < NODES_PER_LAYER; ++i) {
                volatile int result = calc_layers.back()[i].get();
                (void)result;
            }
        }
        auto batch_time = timer.stop_microseconds();
        timer.print_result("With batch (complex graph)", batch_time);
        
        double improvement = (double)no_batch_time / batch_time;
        std::cout << "Performance improvement: " << improvement << "x" << std::endl;
    }
}

void analyze_batch_limitations() {
    std::cout << "\n=== Batch Performance Limitations Analysis ===" << std::endl;
    
    std::cout << "\n批处理性能提升有限的情况：" << std::endl;
    std::cout << "1. **线性依赖链**: 每个节点都必须按顺序更新，无法并行化" << std::endl;
    std::cout << "2. **单次更新**: 只更新一个节点时，批处理开销可能大于收益" << std::endl;
    std::cout << "3. **深度优先计算**: 计算复杂度远大于通知开销时" << std::endl;
    std::cout << "4. **内存密集型操作**: 受内存带宽限制而非CPU限制" << std::endl;
    
    std::cout << "\n批处理性能提升显著的情况：" << std::endl;
    std::cout << "1. **扇出结构**: 一个源节点有多个观察者" << std::endl;
    std::cout << "2. **批量更新**: 同时更新多个独立源节点" << std::endl;
    std::cout << "3. **重复依赖**: 多个计算节点依赖相同的源" << std::endl;
    std::cout << "4. **浅层计算**: 计算简单但通知开销大" << std::endl;
    
    std::cout << "\n改进建议：" << std::endl;
    std::cout << "1. **智能批处理**: 自动检测是否值得使用批处理" << std::endl;
    std::cout << "2. **异步通知**: 将通知操作异步化" << std::endl;
    std::cout << "3. **惰性计算**: 延迟计算直到真正需要结果" << std::endl;
    std::cout << "4. **缓存优化**: 避免重复计算相同的值" << std::endl;
}

// 改进的批处理实现建议
class SmartBatch {
public:
    template<typename F>
    static void execute_if_beneficial(F&& f, size_t estimated_operations = 1) {
        if (estimated_operations > BATCH_THRESHOLD) {
            // 使用批处理
            batchExecute(std::forward<F>(f));
        } else {
            // 直接执行
            f();
        }
    }
    
    // 自适应批处理
    template<typename F>
    static void adaptive_execute(F&& f) {
        static thread_local size_t success_count = 0;
        static thread_local size_t total_count = 0;
        
        auto start = high_resolution_clock::now();
        
        if (should_use_batch()) {
            batchExecute(std::forward<F>(f));
        } else {
            f();
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        
        // 根据性能反馈调整策略
        update_strategy(duration);
    }
    
private:
    static constexpr size_t BATCH_THRESHOLD = 5;
    static thread_local double batch_success_rate;
    
    static bool should_use_batch() {
        return batch_success_rate > 0.6; // 60%的情况下批处理更快
    }
    
    static void update_strategy(long long duration) {
        // 实现自适应逻辑
        // 这里简化处理
    }
};

thread_local double SmartBatch::batch_success_rate = 0.5;

int main() {
    test_batch_performance_scenarios();
    analyze_batch_limitations();
    
    std::cout << "\n=== 结论 ===" << std::endl;
    std::cout << "批处理不是万能的性能优化手段，需要根据具体的依赖图结构" << std::endl;
    std::cout << "和使用模式来决定是否使用。框架应该提供智能的批处理策略。" << std::endl;
    
    return 0;
}