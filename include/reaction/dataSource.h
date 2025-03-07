// include/reaction/DATASOURCE.h
#ifndef REACTION_DATASOURCE_H
#define REACTION_DATASOURCE_H

#include "reaction/expression.h"
#include "reaction/valueCache.h"
#include "reaction/observerHelper.h"
#include "reaction/resource.h"
#include <tuple>
#include <functional>
#include <vector>
#include <memory>

namespace reaction
{
    template <typename Fun, typename... Args>
        requires requires(Fun f, Args... args)
    {
        std::invoke(f, args...);
    }
    class DataSource : public Expression<Fun, Args...>
    {
    public:
        using Observer = std::function<void()>;

        // Constructor
        template <typename F, typename... A>
        DataSource(F &&f, A &&...args) : Expression<Fun, Args...>(std::forward<F>(f), std::forward<A>(args)...) {}
    };

    template <typename Fun, typename... Args>
    auto makeDataSource(Fun &&fun, Args &&...args)
    {
        return std::make_shared<DataSource<std::decay_t<Fun>,std::decay_t<Args>...>>(std::forward<Fun>(fun), std::forward<Args>(args)...);
    }

    template <typename Fun, typename... Args>
    DataSource(Fun &&, Args &&...) -> DataSource<std::decay_t<Fun>, std::decay_t<Args>...>;

} // namespace reaction

#endif // REACTION_DATASOURCE_H