/*
 * Exception Handling Design for Reaction Framework
 * 
 * 展示几种异常处理策略的设计和实现
 */

#include "reaction/reaction.h"
#include <iostream>
#include <variant>
#include <optional>
#include <functional>

// 方案1: Result<T, E> 模式 (类似Rust)
template<typename T, typename E = std::exception_ptr>
class Result {
public:
    // 成功结果
    static Result success(T value) {
        return Result(std::move(value));
    }
    
    // 错误结果
    static Result error(E err) {
        return Result(std::move(err));
    }
    
    bool is_success() const { return std::holds_alternative<T>(m_data); }
    bool is_error() const { return std::holds_alternative<E>(m_data); }
    
    const T& value() const { 
        if (!is_success()) throw std::runtime_error("Accessing error result");
        return std::get<T>(m_data); 
    }
    
    const E& error() const { 
        if (!is_error()) throw std::runtime_error("Accessing success result");
        return std::get<E>(m_data); 
    }
    
    // 提供默认值
    T value_or(const T& default_val) const {
        return is_success() ? value() : default_val;
    }
    
private:
    explicit Result(T val) : m_data(std::move(val)) {}
    explicit Result(E err) : m_data(std::move(err)) {}
    
    std::variant<T, E> m_data;
};

// 方案2: 异常策略接口
class ExceptionStrategy {
public:
    virtual ~ExceptionStrategy() = default;
    virtual void handle_exception(std::exception_ptr ex) = 0;
    virtual bool should_propagate() const = 0;
};

// 策略1: 记录并继续（使用默认值）
template<typename T>
class LogAndContinueStrategy : public ExceptionStrategy {
public:
    LogAndContinueStrategy(T default_value) : m_default(std::move(default_value)) {}
    
    void handle_exception(std::exception_ptr ex) override {
        try {
            std::rethrow_exception(ex);
        } catch (const std::exception& e) {
            std::cerr << "[EXCEPTION] Calculation failed: " << e.what() 
                     << ", using default value: " << m_default << std::endl;
        }
        m_has_error = true;
    }
    
    bool should_propagate() const override { return false; }
    
    T get_default() const { return m_default; }
    bool has_error() const { return m_has_error; }
    
private:
    T m_default;
    bool m_has_error = false;
};

// 策略2: 记录并传播
class LogAndPropagateStrategy : public ExceptionStrategy {
public:
    void handle_exception(std::exception_ptr ex) override {
        try {
            std::rethrow_exception(ex);
        } catch (const std::exception& e) {
            std::cerr << "[EXCEPTION] Calculation failed: " << e.what() 
                     << ", propagating exception" << std::endl;
        }
    }
    
    bool should_propagate() const override { return true; }
};

// 策略3: 静默失败（返回上次有效值）
template<typename T>
class SilentFailStrategy : public ExceptionStrategy {
public:
    SilentFailStrategy(T last_valid) : m_last_valid(std::move(last_valid)) {}
    
    void handle_exception(std::exception_ptr ex) override {
        // 静默处理，不输出错误信息
        m_has_error = true;
    }
    
    bool should_propagate() const override { return false; }
    
    T get_last_valid() const { return m_last_valid; }
    void update_last_valid(const T& value) { m_last_valid = value; m_has_error = false; }
    bool has_error() const { return m_has_error; }
    
private:
    T m_last_valid;
    bool m_has_error = false;
};

// 改进的CalcExprBase示例（伪代码）
template <typename Type, typename Strategy = LogAndContinueStrategy<Type>>
class SafeCalcExprBase {
public:
    template<typename F, typename... Args>
    void setSource(F&& f, Args&&... args) {
        m_fun = [f = std::forward<F>(f), ...args = args.getPtr()]() -> Result<Type> {
            try {
                if constexpr (std::is_void_v<Type>) {
                    std::invoke(f, args->get()...);
                    return Result<Type>::success(Type{});
                } else {
                    auto result = std::invoke(f, args->get()...);
                    return Result<Type>::success(result);
                }
            } catch (...) {
                return Result<Type>::error(std::current_exception());
            }
        };
    }
    
    Type getValue() {
        auto result = m_fun();
        if (result.is_success()) {
            if constexpr (requires { m_strategy.update_last_valid(result.value()); }) {
                m_strategy.update_last_valid(result.value());
            }
            return result.value();
        } else {
            m_strategy.handle_exception(result.error());
            if (m_strategy.should_propagate()) {
                std::rethrow_exception(result.error());
            }
            
            // 返回策略提供的值
            if constexpr (requires { m_strategy.get_default(); }) {
                return m_strategy.get_default();
            } else if constexpr (requires { m_strategy.get_last_valid(); }) {
                return m_strategy.get_last_valid();
            } else {
                return Type{}; // 默认构造
            }
        }
    }
    
private:
    std::function<Result<Type>()> m_fun;
    Strategy m_strategy;
};

// 使用示例
void demonstrate_exception_strategies() {
    std::cout << "=== Exception Handling Strategies Demo ===" << std::endl;
    
    // 策略1: 使用默认值继续
    {
        std::cout << "\n1. Log and Continue Strategy:" << std::endl;
        
        auto source = var(1);
        // 假设我们有支持策略的calc
        // auto safe_calc = calc_with_strategy<LogAndContinueStrategy<int>>(
        //     [](int x) -> int {
        //         if (x == 42) throw std::runtime_error("Test exception");
        //         return x * 2;
        //     }, 
        //     source,
        //     LogAndContinueStrategy<int>(999) // 默认值999
        // );
        
        // 模拟行为
        try {
            throw std::runtime_error("Test exception");
        } catch (...) {
            LogAndContinueStrategy<int> strategy(999);
            strategy.handle_exception(std::current_exception());
            std::cout << "Using default value: " << strategy.get_default() << std::endl;
        }
    }
    
    // 策略2: 记录并传播
    {
        std::cout << "\n2. Log and Propagate Strategy:" << std::endl;
        
        try {
            throw std::runtime_error("Test exception");
        } catch (...) {
            LogAndPropagateStrategy strategy;
            strategy.handle_exception(std::current_exception());
            if (strategy.should_propagate()) {
                std::cout << "Exception will be propagated to caller" << std::endl;
            }
        }
    }
    
    // 策略3: 使用上次有效值
    {
        std::cout << "\n3. Silent Fail Strategy:" << std::endl;
        
        SilentFailStrategy<int> strategy(100); // 上次有效值100
        
        try {
            throw std::runtime_error("Test exception");
        } catch (...) {
            strategy.handle_exception(std::current_exception());
            std::cout << "Using last valid value: " << strategy.get_last_valid() << std::endl;
            std::cout << "Has error: " << strategy.has_error() << std::endl;
        }
    }
}

// 全局异常处理器
class GlobalExceptionHandler {
public:
    static GlobalExceptionHandler& instance() {
        static GlobalExceptionHandler handler;
        return handler;
    }
    
    void set_handler(std::function<void(std::exception_ptr)> handler) {
        m_handler = std::move(handler);
    }
    
    void handle_exception(std::exception_ptr ex) {
        if (m_handler) {
            m_handler(ex);
        } else {
            default_handler(ex);
        }
    }
    
private:
    void default_handler(std::exception_ptr ex) {
        try {
            std::rethrow_exception(ex);
        } catch (const std::exception& e) {
            std::cerr << "[GLOBAL HANDLER] Unhandled exception: " << e.what() << std::endl;
        }
    }
    
    std::function<void(std::exception_ptr)> m_handler;
};

int main() {
    // 设置全局异常处理器
    GlobalExceptionHandler::instance().set_handler([](std::exception_ptr ex) {
        try {
            std::rethrow_exception(ex);
        } catch (const std::exception& e) {
            std::cout << "[CUSTOM HANDLER] Caught: " << e.what() << std::endl;
            // 可以记录日志、发送警报等
        }
    });
    
    demonstrate_exception_strategies();
    
    return 0;
}