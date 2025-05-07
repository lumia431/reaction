#include "reaction/resource.h"
#include <tuple>
#include <utility>
namespace reaction
{
    struct VarExpr
    {
    };
    struct CalcExpr
    {
    };

    template <typename Fun, typename... Args>
    class Expression : public Resource<ReturnType<Fun, Args...>>
    {
    public:
        using ValueType = ReturnType<Fun, Args...>;
        using ExprType = CalcExpr;

        template <typename F, typename... A>
        Expression(F &&fun, A &&...args)
            : Resource<ReturnType<Fun, Args...>>(), m_fun(std::forward<F>(fun)), m_args(std::forward<A>(args)...)
        {
            this->updateObservers(std::forward<A>(args)...);
            evaluate();
        }

    private:
        void valueChanged() override
        {
            evaluate();
        }

        void evaluate()
        {
            if constexpr (VoidType<ValueType>)
            {
                [&]<std::size_t...I>(std::index_sequence<I...>)
                {
                    std::invoke(m_fun, std::get<I>(m_args).get().get()...);
                }(std::make_index_sequence<std::tuple_size_v<decltype(m_args)>>{});
            }
            else
            {
                auto result = [&]<std::size_t...I>(std::index_sequence<I...>)
                {
                    return std::invoke(m_fun, std::get<I>(m_args).get().get()...);
                }(std::make_index_sequence<std::tuple_size_v<decltype(m_args)>>{});
    
                this->updateValue(result);
            }

        }
        Fun m_fun;
        std::tuple<std::reference_wrapper<Args>...> m_args;
    };

    template <typename Type>
    class Expression<Type> : public Resource<Type>
    {
    public:
        using ValueType = Type;
        using ExprType = VarExpr;
        using Resource<Type>::Resource;
    };
}