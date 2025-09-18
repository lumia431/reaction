/*
 * Solutions for Dangling Reference Detection in Reaction Framework
 * 
 * 提供多种方案来避免和检测悬挂引用问题
 */

#include <memory>
#include <functional>
#include <iostream>
#include <unordered_set>
#include <typeinfo>
#include <type_traits>

// 方案1: 智能捕获包装器
template<typename T>
class SafeCapture {
public:
    // 按值捕获
    explicit SafeCapture(T value) : m_data(std::make_shared<T>(std::move(value))) {}
    
    // 禁止引用捕获
    SafeCapture(T&) = delete;
    SafeCapture(const T&) = delete;
    
    const T& get() const { 
        if (!m_data) {
            throw std::runtime_error("Accessing destroyed captured value");
        }
        return *m_data; 
    }
    
    bool is_valid() const { return static_cast<bool>(m_data); }
    
private:
    std::shared_ptr<T> m_data;
};

// 方案2: 生命周期追踪器
class LifetimeTracker {
public:
    class Token {
    public:
        Token() : m_id(++s_next_id) {
            s_active_tokens.insert(m_id);
        }
        
        ~Token() {
            s_active_tokens.erase(m_id);
        }
        
        // 禁止拷贝，只允许移动
        Token(const Token&) = delete;
        Token& operator=(const Token&) = delete;
        
        Token(Token&& other) noexcept : m_id(other.m_id) {
            other.m_id = 0;
        }
        
        Token& operator=(Token&& other) noexcept {
            if (this != &other) {
                if (m_id != 0) {
                    s_active_tokens.erase(m_id);
                }
                m_id = other.m_id;
                other.m_id = 0;
            }
            return *this;
        }
        
        bool is_valid() const {
            return m_id != 0 && s_active_tokens.contains(m_id);
        }
        
        uint64_t id() const { return m_id; }
        
    private:
        uint64_t m_id;
        static std::atomic<uint64_t> s_next_id;
        static std::unordered_set<uint64_t> s_active_tokens;
    };
    
    template<typename T>
    class TrackedReference {
    public:
        TrackedReference(T& ref, std::shared_ptr<Token> token) 
            : m_ref(&ref), m_token(std::move(token)) {}
        
        const T& get() const {
            if (!m_token || !m_token->is_valid()) {
                throw std::runtime_error("Accessing dangling reference");
            }
            return *m_ref;
        }
        
        bool is_valid() const {
            return m_token && m_token->is_valid();
        }
        
    private:
        T* m_ref;
        std::shared_ptr<Token> m_token;
    };
    
    template<typename T>
    static TrackedReference<T> track(T& ref) {
        auto token = std::make_shared<Token>();
        return TrackedReference<T>(ref, token);
    }
};

std::atomic<uint64_t> LifetimeTracker::Token::s_next_id{0};
std::unordered_set<uint64_t> LifetimeTracker::Token::s_active_tokens;

// 方案3: 作用域守卫
template<typename T>
class ScopeGuard {
public:
    explicit ScopeGuard(T& ref) : m_ref(&ref), m_valid(true) {}
    
    ~ScopeGuard() {
        m_valid = false;
    }
    
    // 禁止拷贝和移动
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;
    
    const T& get() const {
        if (!m_valid) {
            throw std::runtime_error("Scope guard invalidated");
        }
        return *m_ref;
    }
    
    bool is_valid() const { return m_valid; }
    
private:
    T* m_ref;
    bool m_valid;
};

// 方案4: 安全Lambda工厂
template<typename F>
class SafeLambdaFactory {
public:
    // 强制按值捕获的lambda创建器
    template<typename... Args>
    static auto make_safe_lambda(F&& f, Args... args) {
        // 将所有参数按值捕获
        return [f = std::forward<F>(f), args...](auto&&... call_args) {
            return f(args..., std::forward<decltype(call_args)>(call_args)...);
        };
    }
    
    // 检测引用捕获的编译时警告
    template<typename T>
    static constexpr bool is_reference_capture() {
        return std::is_reference_v<T>;
    }
};

// 方案5: 智能函数包装器
template<typename Sig>
class SafeFunction;

template<typename R, typename... Args>
class SafeFunction<R(Args...)> {
public:
    template<typename F>
    SafeFunction(F&& f) {
        static_assert(!std::is_same_v<std::decay_t<F>, SafeFunction>, 
                     "Avoid recursive SafeFunction construction");
        
        // 检查是否可能有悬挂引用
        validate_function(std::forward<F>(f));
        
        m_function = [f = std::forward<F>(f)](Args... args) -> R {
            try {
                if constexpr (std::is_void_v<R>) {
                    f(args...);
                } else {
                    return f(args...);
                }
            } catch (const std::exception& e) {
                std::cerr << "Exception in safe function: " << e.what() << std::endl;
                if constexpr (!std::is_void_v<R>) {
                    throw;
                }
            }
        };
    }
    
    R operator()(Args... args) const {
        if (!m_function) {
            throw std::runtime_error("SafeFunction is not initialized");
        }
        return m_function(args...);
    }
    
    bool is_valid() const { return static_cast<bool>(m_function); }
    
private:
    template<typename F>
    void validate_function(F&& f) {
        // 这里可以添加编译时或运行时检查
        // 例如：检查lambda的捕获列表
        std::cout << "Validating function of type: " << typeid(F).name() << std::endl;
    }
    
    std::function<R(Args...)> m_function;
};

// 使用示例和测试
void demonstrate_dangling_solutions() {
    std::cout << "=== Dangling Reference Solutions Demo ===" << std::endl;
    
    // 方案1: 安全捕获
    {
        std::cout << "\n1. Safe Capture Solution:" << std::endl;
        
        std::function<int()> safe_lambda;
        
        {
            int local_value = 42;
            // 错误的方式（会导致悬挂引用）:
            // auto bad_lambda = [&local_value]() { return local_value; };
            
            // 安全的方式:
            SafeCapture<int> captured(local_value);
            safe_lambda = [captured]() { return captured.get(); };
            
            std::cout << "Inside scope: " << safe_lambda() << std::endl;
        }
        
        // local_value已经超出作用域
        try {
            std::cout << "Outside scope: " << safe_lambda() << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Caught expected exception: " << e.what() << std::endl;
        }
    }
    
    // 方案2: 生命周期追踪
    {
        std::cout << "\n2. Lifetime Tracking Solution:" << std::endl;
        
        std::function<int()> tracked_lambda;
        
        {
            int local_value = 100;
            auto tracked_ref = LifetimeTracker::track(local_value);
            
            tracked_lambda = [tracked_ref]() { return tracked_ref.get(); };
            
            std::cout << "Inside scope: " << tracked_lambda() << std::endl;
            std::cout << "Reference valid: " << tracked_ref.is_valid() << std::endl;
        }
        
        // 作用域结束后
        try {
            std::cout << "Outside scope: " << tracked_lambda() << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Caught expected exception: " << e.what() << std::endl;
        }
    }
    
    // 方案3: 作用域守卫
    {
        std::cout << "\n3. Scope Guard Solution:" << std::endl;
        
        std::function<int()> guarded_lambda;
        
        {
            int local_value = 200;
            ScopeGuard<int> guard(local_value);
            
            // 注意：这里仍然有问题，因为guard也会超出作用域
            // 这个方案主要用于检测而不是解决问题
            std::cout << "Guard valid: " << guard.is_valid() << std::endl;
            std::cout << "Value: " << guard.get() << std::endl;
        }
    }
    
    // 方案4: 安全Lambda工厂
    {
        std::cout << "\n4. Safe Lambda Factory Solution:" << std::endl;
        
        int local_value = 300;
        
        // 创建安全的lambda（按值捕获）
        auto safe_lambda = SafeLambdaFactory<std::function<int(int)>>::make_safe_lambda(
            [](int captured_val, int multiplier) { return captured_val * multiplier; },
            local_value  // 按值传递
        );
        
        std::cout << "Safe lambda result: " << safe_lambda(2) << std::endl;
    }
    
    // 方案5: 智能函数包装器
    {
        std::cout << "\n5. Safe Function Wrapper Solution:" << std::endl;
        
        int local_value = 400;
        
        SafeFunction<int()> safe_func([local_value]() { return local_value * 2; });
        
        std::cout << "Safe function result: " << safe_func() << std::endl;
        std::cout << "Function valid: " << safe_func.is_valid() << std::endl;
    }
}

// 编译时检查辅助宏
#define SAFE_CAPTURE(var) SafeCapture(var)
#define TRACK_LIFETIME(var) LifetimeTracker::track(var)

// 静态分析辅助（概念验证）
template<typename Lambda>
constexpr bool has_reference_capture() {
    // 这是一个简化的检查，实际实现会更复杂
    return false; // 需要更复杂的模板元编程来实现
}

int main() {
    demonstrate_dangling_solutions();
    
    std::cout << "\n=== Best Practices ===" << std::endl;
    std::cout << "1. 优先使用按值捕获" << std::endl;
    std::cout << "2. 使用智能指针管理生命周期" << std::endl;
    std::cout << "3. 提供编译时检查工具" << std::endl;
    std::cout << "4. 运行时检测和错误报告" << std::endl;
    std::cout << "5. 清晰的API设计指导用户正确使用" << std::endl;
    
    return 0;
}