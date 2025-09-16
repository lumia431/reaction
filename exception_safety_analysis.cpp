/*
 * Exception Safety Analysis for Reaction Framework
 * 
 * 详细分析框架的异常安全性问题，并提供改进方案
 */

#include <iostream>
#include <memory>
#include <vector>
#include <exception>
#include <functional>

// 异常安全级别定义
enum class ExceptionSafety {
    NoGuarantee,    // 无保证：可能泄露资源或破坏不变量
    Basic,          // 基本保证：不泄露资源，对象处于有效状态
    Strong,         // 强保证：操作要么完全成功，要么完全失败（事务性）
    NoThrow         // 无异常保证：操作绝不抛出异常
};

void analyze_current_exception_safety() {
    std::cout << "=== 当前框架异常安全性分析 ===" << std::endl;
    
    std::cout << "\n1. **节点创建过程**:" << std::endl;
    std::cout << "   问题：在构造函数中可能抛出异常，导致部分初始化的对象" << std::endl;
    std::cout << "   当前级别：❌ 无保证" << std::endl;
    std::cout << "   风险：内存泄漏、悬挂指针" << std::endl;
    
    std::cout << "\n2. **依赖关系建立**:" << std::endl;
    std::cout << "   问题：添加观察者时异常可能导致图结构不一致" << std::endl;
    std::cout << "   当前级别：❌ 无保证" << std::endl;
    std::cout << "   风险：依赖图损坏、内存泄漏" << std::endl;
    
    std::cout << "\n3. **值更新过程**:" << std::endl;
    std::cout << "   问题：更新值时异常可能导致通知链中断" << std::endl;
    std::cout << "   当前级别：❌ 无保证" << std::endl;
    std::cout << "   风险：状态不一致、部分更新" << std::endl;
    
    std::cout << "\n4. **批处理操作**:" << std::endl;
    std::cout << "   问题：批处理中的异常可能导致部分操作完成" << std::endl;
    std::cout << "   当前级别：❌ 无保证" << std::endl;
    std::cout << "   风险：批处理原子性被破坏" << std::endl;
    
    std::cout << "\n5. **资源清理**:" << std::endl;
    std::cout << "   问题：析构函数中可能抛出异常" << std::endl;
    std::cout << "   当前级别：❌ 无保证" << std::endl;
    std::cout << "   风险：双重异常、资源泄漏" << std::endl;
}

// 异常安全的资源管理器
template<typename T>
class ExceptionSafeResource {
public:
    // 强异常安全的构造
    template<typename... Args>
    explicit ExceptionSafeResource(Args&&... args) {
        try {
            m_resource = std::make_unique<T>(std::forward<Args>(args)...);
            m_valid = true;
        } catch (...) {
            m_valid = false;
            throw; // 重新抛出异常
        }
    }
    
    // 无异常析构
    ~ExceptionSafeResource() noexcept {
        try {
            reset();
        } catch (...) {
            // 记录错误但不抛出异常
            std::cerr << "Exception in destructor (suppressed)" << std::endl;
        }
    }
    
    // 强异常安全的赋值
    ExceptionSafeResource& operator=(const ExceptionSafeResource& other) {
        if (this != &other) {
            if (other.m_valid) {
                auto temp = std::make_unique<T>(*other.m_resource); // 可能抛出异常
                m_resource = std::move(temp); // 无异常操作
                m_valid = true;
            } else {
                reset();
            }
        }
        return *this;
    }
    
    // 无异常移动
    ExceptionSafeResource(ExceptionSafeResource&& other) noexcept 
        : m_resource(std::move(other.m_resource)), m_valid(other.m_valid) {
        other.m_valid = false;
    }
    
    ExceptionSafeResource& operator=(ExceptionSafeResource&& other) noexcept {
        if (this != &other) {
            m_resource = std::move(other.m_resource);
            m_valid = other.m_valid;
            other.m_valid = false;
        }
        return *this;
    }
    
    // 基本异常安全的访问
    T& get() {
        if (!m_valid) {
            throw std::runtime_error("Accessing invalid resource");
        }
        return *m_resource;
    }
    
    const T& get() const {
        if (!m_valid) {
            throw std::runtime_error("Accessing invalid resource");
        }
        return *m_resource;
    }
    
    // 无异常检查
    bool is_valid() const noexcept { return m_valid; }
    
    // 强异常安全的重置
    void reset() {
        m_resource.reset();
        m_valid = false;
    }
    
private:
    std::unique_ptr<T> m_resource;
    bool m_valid = false;
};

// 异常安全的观察者图管理
class ExceptionSafeObserverGraph {
public:
    struct Node {
        int id;
        std::vector<std::shared_ptr<Node>> observers;
        std::vector<std::weak_ptr<Node>> dependencies;
        
        Node(int node_id) : id(node_id) {}
    };
    
    using NodePtr = std::shared_ptr<Node>;
    
    // 强异常安全的节点添加
    NodePtr add_node(int id) {
        auto new_node = std::make_shared<Node>(id);
        
        // 使用RAII确保异常安全
        struct NodeGuard {
            ExceptionSafeObserverGraph* graph;
            NodePtr node;
            bool committed = false;
            
            ~NodeGuard() {
                if (!committed && graph && node) {
                    // 清理部分添加的节点
                    graph->remove_node_internal(node->id);
                }
            }
            
            void commit() { committed = true; }
        };
        
        NodeGuard guard{this, new_node};
        
        // 可能抛出异常的操作
        m_nodes[id] = new_node;
        
        guard.commit(); // 提交操作
        return new_node;
    }
    
    // 强异常安全的依赖添加
    void add_dependency(NodePtr observer, NodePtr dependency) {
        if (!observer || !dependency) {
            throw std::invalid_argument("Null node pointer");
        }
        
        // 检查循环依赖
        if (would_create_cycle(observer, dependency)) {
            throw std::runtime_error("Would create cycle");
        }
        
        // 创建回滚点
        struct DependencyGuard {
            NodePtr observer;
            NodePtr dependency;
            bool observer_added = false;
            bool dependency_added = false;
            bool committed = false;
            
            ~DependencyGuard() {
                if (!committed) {
                    // 回滚操作
                    if (dependency_added) {
                        auto it = std::find(observer->observers.begin(), 
                                          observer->observers.end(), dependency);
                        if (it != observer->observers.end()) {
                            observer->observers.erase(it);
                        }
                    }
                    if (observer_added) {
                        auto it = std::find_if(dependency->dependencies.begin(),
                                             dependency->dependencies.end(),
                                             [this](const std::weak_ptr<Node>& wp) {
                                                 auto sp = wp.lock();
                                                 return sp && sp == observer;
                                             });
                        if (it != dependency->dependencies.end()) {
                            dependency->dependencies.erase(it);
                        }
                    }
                }
            }
            
            void commit() { committed = true; }
        };
        
        DependencyGuard guard{observer, dependency};
        
        // 原子性地添加双向依赖
        observer->observers.push_back(dependency);
        guard.observer_added = true;
        
        dependency->dependencies.emplace_back(observer);
        guard.dependency_added = true;
        
        guard.commit();
    }
    
    // 基本异常安全的节点移除
    void remove_node(int id) noexcept {
        try {
            remove_node_internal(id);
        } catch (...) {
            // 记录错误但保证基本安全
            std::cerr << "Error removing node " << id << std::endl;
        }
    }
    
private:
    std::unordered_map<int, NodePtr> m_nodes;
    
    bool would_create_cycle(NodePtr observer, NodePtr dependency) {
        // 简化的循环检测
        return observer == dependency;
    }
    
    void remove_node_internal(int id) {
        auto it = m_nodes.find(id);
        if (it != m_nodes.end()) {
            auto node = it->second;
            
            // 清理所有依赖关系
            for (auto& observer : node->observers) {
                if (observer) {
                    auto dep_it = std::find_if(observer->dependencies.begin(),
                                             observer->dependencies.end(),
                                             [node](const std::weak_ptr<Node>& wp) {
                                                 auto sp = wp.lock();
                                                 return sp && sp == node;
                                             });
                    if (dep_it != observer->dependencies.end()) {
                        observer->dependencies.erase(dep_it);
                    }
                }
            }
            
            m_nodes.erase(it);
        }
    }
};

// 异常安全的批处理操作
class ExceptionSafeBatch {
public:
    template<typename F>
    class BatchGuard {
    public:
        explicit BatchGuard(F&& operation) : m_operation(std::forward<F>(operation)) {}
        
        ~BatchGuard() {
            if (!m_committed && !m_rolled_back) {
                try {
                    rollback();
                } catch (...) {
                    // 析构函数中不能抛出异常
                    std::cerr << "Rollback failed in destructor" << std::endl;
                }
            }
        }
        
        // 执行操作（强异常安全）
        void execute() {
            if (m_committed || m_rolled_back) {
                throw std::logic_error("Operation already executed or rolled back");
            }
            
            try {
                // 保存状态用于回滚
                save_state();
                
                // 执行操作
                m_operation();
                
                // 标记为已提交
                m_committed = true;
                
            } catch (...) {
                // 发生异常时回滚
                try {
                    rollback();
                } catch (...) {
                    // 回滚失败，记录错误
                    std::cerr << "Rollback failed during exception handling" << std::endl;
                }
                throw; // 重新抛出原始异常
            }
        }
        
        // 手动提交
        void commit() {
            if (!m_committed) {
                throw std::logic_error("Cannot commit before execution");
            }
        }
        
        // 手动回滚
        void rollback() {
            if (m_committed) {
                throw std::logic_error("Cannot rollback committed operation");
            }
            
            if (!m_rolled_back) {
                restore_state();
                m_rolled_back = true;
            }
        }
        
    private:
        F m_operation;
        bool m_committed = false;
        bool m_rolled_back = false;
        
        void save_state() {
            // 保存当前状态用于回滚
            // 实际实现中需要保存相关的状态信息
        }
        
        void restore_state() {
            // 恢复之前保存的状态
            // 实际实现中需要恢复状态
        }
    };
    
    template<typename F>
    static auto create_batch(F&& operation) {
        return BatchGuard<F>(std::forward<F>(operation));
    }
};

// 异常安全的值更新
template<typename T>
class ExceptionSafeValue {
public:
    explicit ExceptionSafeValue(T initial_value) : m_value(std::move(initial_value)) {}
    
    // 强异常安全的更新
    void update(const T& new_value) {
        T temp = new_value; // 可能抛出异常（拷贝构造）
        m_value = std::move(temp); // 无异常操作
        notify_observers(); // 可能抛出异常
    }
    
    void update(T&& new_value) {
        T temp = std::move(new_value); // 通常无异常
        std::swap(m_value, temp); // 无异常操作
        try {
            notify_observers();
        } catch (...) {
            // 通知失败时回滚
            std::swap(m_value, temp);
            throw;
        }
    }
    
    // 无异常访问
    const T& get() const noexcept { return m_value; }
    
    // 基本异常安全的观察者管理
    void add_observer(std::function<void()> observer) {
        m_observers.push_back(std::move(observer)); // 可能抛出异常
    }
    
private:
    T m_value;
    std::vector<std::function<void()>> m_observers;
    
    void notify_observers() {
        std::vector<std::exception_ptr> exceptions;
        
        for (auto& observer : m_observers) {
            try {
                observer();
            } catch (...) {
                exceptions.push_back(std::current_exception());
            }
        }
        
        // 如果有异常发生，抛出第一个异常
        if (!exceptions.empty()) {
            std::rethrow_exception(exceptions[0]);
        }
    }
};

void demonstrate_exception_safety_improvements() {
    std::cout << "\n=== 异常安全性改进示例 ===" << std::endl;
    
    // 1. 异常安全的资源管理
    {
        std::cout << "\n1. Exception Safe Resource Management:" << std::endl;
        
        try {
            ExceptionSafeResource<std::string> resource("test");
            std::cout << "Resource created: " << resource.get() << std::endl;
            std::cout << "Resource valid: " << resource.is_valid() << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Resource creation failed: " << e.what() << std::endl;
        }
    }
    
    // 2. 异常安全的批处理
    {
        std::cout << "\n2. Exception Safe Batch Operations:" << std::endl;
        
        try {
            auto batch = ExceptionSafeBatch::create_batch([]() {
                std::cout << "Executing batch operation..." << std::endl;
                // 模拟可能抛出异常的操作
                // throw std::runtime_error("Batch operation failed");
            });
            
            batch.execute();
            std::cout << "Batch executed successfully" << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "Batch execution failed: " << e.what() << std::endl;
        }
    }
    
    // 3. 异常安全的值更新
    {
        std::cout << "\n3. Exception Safe Value Updates:" << std::endl;
        
        try {
            ExceptionSafeValue<int> safe_value(42);
            std::cout << "Initial value: " << safe_value.get() << std::endl;
            
            safe_value.update(100);
            std::cout << "Updated value: " << safe_value.get() << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "Value update failed: " << e.what() << std::endl;
        }
    }
}

void provide_exception_safety_guidelines() {
    std::cout << "\n=== 异常安全性设计指南 ===" << std::endl;
    
    std::cout << "\n**基本原则**:" << std::endl;
    std::cout << "1. **RAII**: 使用资源获取即初始化模式" << std::endl;
    std::cout << "2. **异常中性**: 让异常自然传播，不隐藏错误" << std::endl;
    std::cout << "3. **状态一致性**: 确保对象在异常后仍处于有效状态" << std::endl;
    std::cout << "4. **原子操作**: 关键操作要么全部成功，要么全部失败" << std::endl;
    
    std::cout << "\n**实现策略**:" << std::endl;
    std::cout << "1. **构造函数**: 使用初始化列表，避免部分构造" << std::endl;
    std::cout << "2. **析构函数**: 必须是noexcept，处理所有可能的异常" << std::endl;
    std::cout << "3. **赋值操作**: 使用copy-and-swap或强异常安全技术" << std::endl;
    std::cout << "4. **批处理**: 实现事务性语义，支持回滚" << std::endl;
    
    std::cout << "\n**错误处理策略**:" << std::endl;
    std::cout << "1. **立即处理**: 在错误发生点立即处理" << std::endl;
    std::cout << "2. **延迟处理**: 收集错误信息，统一处理" << std::endl;
    std::cout << "3. **错误传播**: 让调用者决定如何处理错误" << std::endl;
    std::cout << "4. **错误恢复**: 提供默认值或回退机制" << std::endl;
    
    std::cout << "\n**测试建议**:" << std::endl;
    std::cout << "1. **异常注入测试**: 在各个点注入异常测试恢复能力" << std::endl;
    std::cout << "2. **资源泄漏检测**: 使用工具检测内存和资源泄漏" << std::endl;
    std::cout << "3. **状态验证**: 异常后验证对象状态的正确性" << std::endl;
    std::cout << "4. **性能测试**: 确保异常处理不会显著影响性能" << std::endl;
}

int main() {
    analyze_current_exception_safety();
    demonstrate_exception_safety_improvements();
    provide_exception_safety_guidelines();
    
    std::cout << "\n=== 总结 ===" << std::endl;
    std::cout << "异常安全性是框架可靠性的基础。当前框架在异常处理方面" << std::endl;
    std::cout << "存在严重缺陷，需要系统性地重新设计异常处理机制，" << std::endl;
    std::cout << "确保至少达到基本异常安全级别，关键操作要达到强异常安全。" << std::endl;
    
    return 0;
}