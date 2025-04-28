#include "reaction/resource.h"
#include <tuple>
#include <utility>
namespace reaction
{
    template <typename T, typename... Args>
    class DataSource;

    template <typename T>
    struct ExpressionTraits
    {
        using type = T;
    };

    template <typename T>
    struct ExpressionTraits<DataSource<T>>
    {
        using type = T;
    };

    template <typename Fun, typename... Args>
    struct ExpressionTraits<DataSource<Fun, Args...>>
    {
        using type = std::invoke_result_t<Fun, typename ExpressionTraits<Args>::type...>;
    };

    template <typename Fun, typename... Args>
    using ReturnType = typename ExpressionTraits<DataSource<Fun, Args...>>::type;

    template <typename Fun, typename... Args>
    class Expression : public Resource<ReturnType<Fun, Args...>>
    {
    public:
        template <typename F, typename... A>
        Expression(F &&fun, A &&...args)
            : Resource<ReturnType<Fun, Args...>>(), m_fun(std::forward<F>(fun)), m_args(std::forward<A>(args)...)
        {
            evaluate();
        }
    private:
        void evaluate()
        {
            auto result = [&]<std::size_t...I>(std::index_sequence<I...>)
            {
                return std::invoke(m_fun, std::get<I>(m_args).get().get()...);
            }(std::make_index_sequence<std::tuple_size_v<decltype(m_args)>>{});

            this->updateValue(result);
        }
        Fun m_fun;
        std::tuple<std::reference_wrapper<Args>...> m_args;
    };

    template <typename Type>
    class Expression<Type> : public Resource<Type>
    {
    public:
        using Resource<Type>::Resource;
    };
}