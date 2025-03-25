#ifndef REACTION_EXPRESSION_H
#define REACTION_EXPRESSION_H

#include "reaction/log.h"
#include "reaction/resource.h"
#include "reaction/observerNode.h"
#include "reaction/triggerMode.h"

namespace reaction
{
    // 前置声明
    template <typename TriggerPolicy, typename InvalidStrategy, typename Type, typename... Args>
    class DataSource;

    // 基础模板
    template <typename T>
    struct ExpressionTraits
    {
        using Type = T;
    };

    // DataSource特化（非可调用类型）
    template <typename TriggerPolicy, typename InvalidStrategy, typename T>
        requires(!std::is_invocable_v<T>)
    struct ExpressionTraits<DataSource<TriggerPolicy, InvalidStrategy, T>>
    {
        using Type = T;
    };

    // DataSource特化（可调用类型）
    template <typename TriggerPolicy, typename InvalidStrategy, typename Fun, typename... Args>
    struct ExpressionTraits<DataSource<TriggerPolicy, InvalidStrategy, Fun, Args...>>
    {
        using Type = decltype(std::declval<Fun>()(std::declval<typename ExpressionTraits<Args>::Type>()...));
    };

    // ReturnType别名（最终暴露给外部的接口）
    template <typename Fun, typename... Args>
    using ReturnType = typename ExpressionTraits<DataSource<void, void, Fun, Args...>>::Type;

    // 公共基类模板
    template <typename Derived, typename TriggerPolicy>
    class ExpressionBase : public TriggerPolicy
    {
    protected:
        void valueChanged(bool changed)
        {
            if (TriggerPolicy::checkTrigger(changed))
            {
                static_cast<Derived *>(this)->valueChangedImpl();
            }
        }

        template <typename F, typename... A>
        bool setSourceCommon(F &&f, A &&...args)
        {
            bool repeat = false;
            if (!static_cast<Derived *>(this)->updateObservers(
                    repeat,
                    [this](bool changed)
                    { this->valueChanged(changed); },
                    std::forward<A>(args)...))
            {
                return false;
            }

            if (repeat)
            {
                static_cast<Derived *>(this)->setFunctor(createUpdateFunRef(std::forward<F>(f), std::forward<A>(args)...));
            }
            else
            {
                static_cast<Derived *>(this)->setFunctor(createGetFunRef(std::forward<F>(f), std::forward<A>(args)...));
            }

            if constexpr (std::is_same_v<TriggerPolicy, ThresholdTrigger>)
            {
                TriggerPolicy::setRepeatDependent(repeat);
            }

            return true;
        }
    };

    // 主模板
    template <typename TriggerPolicy, typename Fun, typename... Args>
    class Expression : public Resource<ReturnType<Fun, Args...>>,
                       public ObserverDataNode,
                       public ExpressionBase<Expression<TriggerPolicy, Fun, Args...>, TriggerPolicy>
    {
    public:
        using ValueType = ReturnType<Fun, Args...>;
        friend class ExpressionBase<Expression, TriggerPolicy>;

    protected:
        ValueType evaluate()
        {
            return std::invoke(m_fun);
        }

        void valueChangedImpl()
        {
            auto oldVal = this->getValue();
            auto newVal = evaluate();
            this->updateValue(newVal);
            this->notifyObservers(oldVal != newVal);
        }

        void setFunctor(std::function<ValueType()> fun)
        {
            m_fun = std::move(fun);
        }

        template <typename F, typename... A>
        bool setSource(F &&f, A &&...args)
        {
            if (!this->setSourceCommon(std::forward<F>(f), std::forward<A>(args)...))
            {
                return false;
            }
            this->updateValue(evaluate());
            return true;
        }

    private:
        std::function<ValueType()> m_fun;
    };

    // 特化1: 简单类型
    template <typename TriggerPolicy, typename Type>
    class Expression<TriggerPolicy, Type> : public Resource<std::decay_t<Type>>,
                                            public ObserverDataNode
    {
    public:
        using Resource<std::decay_t<Type>>::Resource;

    protected:
        Type evaluate()
        {
            return this->getValue();
        }

        template <typename T>
        bool setSource(T &&t)
        {
            this->updateValue(std::forward<T>(t));
            return true;
        }
    };

    // 特化2: void类型
    template <typename TriggerPolicy>
    class Expression<TriggerPolicy, void> : public ObserverActionNode,
                                            public ExpressionBase<Expression<TriggerPolicy, void>, TriggerPolicy>
    {
    public:
        friend class ExpressionBase<Expression, TriggerPolicy>;

    protected:
        void evaluate()
        {
            std::invoke(m_fun);
        }

        void valueChangedImpl()
        {
            evaluate();
        }

        void setFunctor(std::function<void()> fun)
        {
            m_fun = std::move(fun);
        }

        template <typename F, typename... A>
        bool setSource(F &&f, A &&...args)
        {
            if (!this->setSourceCommon(std::forward<F>(f), std::forward<A>(args)...))
            {
                return false;
            }
            evaluate();
            return true;
        }

    private:
        std::function<void()> m_fun;
    };

} // namespace reaction

#endif // REACTION_EXPRESSION_H