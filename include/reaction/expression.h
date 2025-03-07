// include/reaction/Expression.h
#ifndef REACTION_EXPRESSION_H
#define REACTION_EXPRESSION_H

#include <tuple>
#include <functional>

namespace reaction
{
    template <typename Fun, typename... Args>
    class Expression
    {
    public:
        using TupleType = std::tuple<Args...>;
        using CallbackType = std::function<void(Args...)>;

        template<typename F, typename... A>
        Expression(F&& f, A&&... args): m_fun(std::forward<F>(f)), m_values(std::forward<A>(args)...){}

        auto evaluate() {
            std::apply(m_fun, m_values);
        }

    private:
        CallbackType m_fun;
        TupleType m_values;
    };

} // namespace reaction

#endif // REACTION_EXPRESSION_H