#ifndef REACTION_EXPRESSION_H
#define REACTION_EXPRESSION_H

#include "reaction/exception.h"
#include "reaction/resource.h"
#include "reaction/observerNode.h"
#include <functional>

namespace reaction
{

    // 前置声明 DataSource
    template <typename Type, typename... Args>
    class DataSource;

    // ExpressionTraits 用于“解包” DataSource 得到实际的值类型
    template <typename T>
    struct ExpressionTraits : std::false_type
    {
        using Type = std::decay_t<T>;
    };

    template <typename T>
        requires(!std::is_invocable_v<std::decay_t<T>>)
    struct ExpressionTraits<DataSource<T>> : std::true_type
    {
        using Type = T;
    };

    template <typename Fun, typename... Args>
    struct ExpressionTraits<DataSource<Fun, Args...>> : std::true_type
    {
        using Type = std::invoke_result_t<std::decay_t<Fun>, typename ExpressionTraits<Args>::Type...>;
    };

    template <typename Fun, typename... Args>
    using ReturnType = std::invoke_result_t<Fun, typename ExpressionTraits<Args>::Type...>;

    //
    // 复杂型 Expression 实现（例如 Expression<Lambda, DataSource<int>, DataSource<double>>）
    // 这里的 ValueType 为 ReturnType<Fun, Args...>
    //
    template <typename Fun, typename... Args>
    class Expression : public Resource<ReturnType<Fun, Args...>>, public ObserverNode
    {
    public:
        using ValueType = ReturnType<Fun, Args...>;

        template <typename F, typename... A>
        Expression(F &&f, A &&...args)
        {
            resetDataSource(std::forward<F>(f), std::forward<A>(args)...);
        }

    protected:
        // 计算表达式的值：先调用各个依赖的 evaluate()，再用 m_fun 组合计算
        ValueType evaluate()
        {
            return std::invoke(m_fun);
        }

        // 不能修改原来的类型，只能重新赋值
        template <typename F, typename... A>
        void resetDataSource(F &&f, A &&...args)
        {
            m_fun = createFun(std::forward<F>(f), std::forward<A>(args)...);
            this->updateObservers(this, args...);
            this->updateValue(evaluate());
        }

    private:
        template <typename F, typename... A>
        std::function<ValueType()> createFun(F &&f, A &&...args)
        {
            return [f = std::forward<F>(f), &args...]() mutable
            {
                return std::invoke(f, args.getUpdate()...);
            };
        }
        std::function<ValueType()> m_fun;
    };

    //
    // 简单型 Expression 实现（例如 Expression<int>），不依赖其他 DataSource
    //
    template <typename Type>
    class Expression<Type> : public Resource<std::decay_t<Type>>, public ObserverNode
    {
    public:
        template <typename T>
        Expression(T &&t) : Resource<std::decay_t<Type>>(std::forward<T>(t)) {}

    protected:
        Type evaluate()
        {
            return this->getValue();
        }

        template <typename T>
        void resetDataSource(T &&t)
        {
            this->updateValue(std::forward<T>(t));
        }
    };

} // namespace reaction

#endif // REACTION_EXPRESSION_H
