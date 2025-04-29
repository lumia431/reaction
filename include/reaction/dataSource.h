#include "reaction/expression.h"
namespace reaction
{
    template <typename Type, typename... Args>
    class DataSource : public Expression<Type, Args...>
    {
    public:
        using ValueType = Expression<Type, Args...>::ValueType;
        using ExprType = Expression<Type, Args...>::ExprType;
        using Expression<Type, Args...>::Expression;
        auto get() const {
           return this->getValue();
        }

        template<typename T>
            requires (std::is_convertible_v<std::decay_t<T>, ValueType> && std::is_same_v<ExprType, VarExpr>)
        void value(T &&t) {
            this->updateValue(std::forward<T>(t));
            this->notify();
        }
    };

    template <typename T>
    auto var(T &&t) {
        return DataSource<T>(std::forward<T>(t));
    }

    template <typename Fun, typename... Args>
    auto calc(Fun &&fun, Args &&...args) {
        return DataSource<std::decay_t<Fun>, std::decay_t<Args>...>(std::forward<Fun>(fun), std::forward<Args>(args)...);
    }
}