// include/reaction/DATASOURCE.h
#ifndef REACTION_DATASOURCE_H
#define REACTION_DATASOURCE_H

#include "reaction/expression.h"

namespace reaction
{
    template <typename T>
    struct DataSourceTraits
    {
        using ExprType = Expression<T>;
    };

    template <typename T>
    struct DataSourceTraits<DataSource<const T>>
    {
        using ExprType = Expression<const T>;
    };

    template <typename Fun, typename... Args>
    struct DataSourceTraits<DataSource<Fun, Args...>>
    {
        using ExprType = Expression<Fun, Args...>;
    };

    template <typename Derived>
    class DataSourceBase : public DataSourceTraits<Derived>::ExprType
    {
    public:
        template <typename T, typename... A>
        DataSourceBase(T &&t, A &&...args) : DataSourceTraits<Derived>::ExprType(std::forward<T>(t), std::forward<A>(args)...) {}

        auto get()
        {
            return static_cast<Derived *>(this)->getValue();
        }

        auto getUpdate()
        {
            return static_cast<Derived *>(this)->evaluate();
        }
    };

    template <typename Type, typename... Args>
    class DataSource : public DataSourceBase<DataSource<Type, Args...>>
    {
    public:
        template <typename T, typename... A>
        explicit DataSource(T &&t, A &&...args) : DataSourceBase<DataSource<Type, Args...>>(std::forward<T>(t), std::forward<A>(args)...) {}

        template <typename T>
        DataSource &operator=(T &&t)
        {
            this->updateValue(std::forward<T>(t));
            this->notifyObservers();
            return *this;
        }

        template <typename T, typename... A>
        void reset(T &&t, A &&...args)
        {
            this->resetDataSource(std::forward<T>(t), std::forward<A>(args)...);
        }

    protected:
        void valueChanged() override
        {
            this->updateValue(this->evaluate());
        }
    };

    template <typename Type>
    class DataSource<const Type> : public DataSourceBase<DataSource<const Type>>
    {
    public:
        template <typename T>
        DataSource(T &&value) : DataSourceBase<DataSource<const Type>>(std::forward<T>(value)) {}

        DataSource &operator=(const DataSource &) = delete;
    };

    template <typename Type, typename... Args>
    DataSource(Type &&, Args &&...) -> DataSource<std::decay_t<Type>, std::decay_t<Args>...>;

    template <typename SourceType>
    auto makeConstantDataSource(const SourceType &value)
    {
        return DataSource<const SourceType>(value);
    }

    template <typename SourceType>
    auto makeMetaDataSource(SourceType &&value)
    {
        return DataSource<SourceType>(std::forward<SourceType>(value));
    }

    template <typename Fun, typename... Args>
    auto makeVariableDataSource(Fun &&fun, Args &&...args)
    {
        return DataSource<std::decay_t<Fun>, std::decay_t<Args>...>(
            std::forward<Fun>(fun), std::forward<Args>(args)...);
    }
} // namespace reaction

#endif // REACTION_DATASOURCE_H