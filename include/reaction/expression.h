#ifndef REACTION_EXPRESSION_H
#define REACTION_EXPRESSION_H

#include "reaction/log.h"
#include "reaction/resource.h"
#include "reaction/observerNode.h"
#include "reaction/timerWheel.h"

namespace reaction
{
    template <typename F, typename... A>
    static auto createFun(F &&f, A &&...args)
    {
        return [f = std::forward<F>(f), &...args = std::forward<A>(args)]() mutable
        {
            return std::invoke(f, args->get()...);
        };
    }

    template <typename Derived>
    class ExpressionTrigger
    {
    public:
        template <typename F, typename... A>
        void setThreshold(F &&f, A &&...args)
        {
            m_thresholdFun = createFun(std::forward<F>(f), std::forward<A>(args)...);
        }

    protected:
        std::function<bool()> m_thresholdFun;
    };

    template <typename Type, typename... Args>
    class DataSource;

    template <typename T>
    struct ExpressionTraits : std::false_type
    {
        using Type = T;
    };

    template <typename T>
        requires(!std::is_invocable_v<T>)
    struct ExpressionTraits<DataSource<T>> : std::true_type
    {
        using Type = T;
    };

    template <typename Fun, typename... Args>
    struct ExpressionTraits<DataSource<Fun, Args...>> : std::true_type
    {
        using Type = std::invoke_result_t<Fun, typename ExpressionTraits<Args>::Type...>;
    };

    template <typename Fun, typename... Args>
    using ReturnType = std::invoke_result_t<Fun, typename ExpressionTraits<Args>::Type...>;

    template <typename Fun, typename... Args>
    class Expression : public Resource<ReturnType<Fun, Args...>>, public ObserverNode, public ExpressionTrigger<Expression<Fun, Args...>>
    {
    public:
        using ValueType = ReturnType<Fun, Args...>;

    protected:
        ValueType evaluate(bool threshold = false)
        {
            if (!threshold || std::invoke(this->m_thresholdFun))
            {
                return std::invoke(m_fun);
            }
            return this->getValue();
        }

        template <typename F, typename... A>
        bool setSource(F &&f, A &&...args)
        {
            m_fun = createFun(std::forward<F>(f), std::forward<A>(args)...);
            if (!this->updateObservers(std::forward<A>(args)...))
            {
                return false;
            }
            this->updateValue(evaluate());
            return true;
        }

    private:
        std::function<ValueType()> m_fun;
    };

    template <typename Type>
    class Expression<Type> : public Resource<std::decay_t<Type>>, public ObserverNode
    {
    public:
        template <typename T>
        Expression(T &&t) : Resource<std::decay_t<Type>>(std::forward<T>(t)) {}

    protected:
        Type evaluate(bool threshold = false)
        {
            return this->getValue();
        }

        template <typename T>
        bool setSource(T &&t)
        {
            this->updateValue(std::forward<T>(t));
        }
    };

    template <>
    class Expression<void> : public ObserverNode, public ExpressionTrigger<Expression<void>>
    {
    public:
        Expression() : ObserverNode(true) {}

    protected:
        void evaluate(bool threshold = false)
        {
            if (!threshold || std::invoke(this->m_thresholdFun))
            {
                std::invoke(m_fun);
            }
        }

        template <typename F, typename... A>
        bool setSource(F &&f, A &&...args)
        {
            m_fun = createFun(std::forward<F>(f), std::forward<A>(args)...);
            if (!this->updateObservers(std::forward<A>(args)...))
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
