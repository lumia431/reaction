#include "reaction/expression.h"
namespace reaction
{
    template <typename T, typename... Args>
    class DataSource : public Expression<T, Args...>
    {
    public:
        using Expression<T, Args...>::Expression;
        auto get() const {
           return this->getValue();
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