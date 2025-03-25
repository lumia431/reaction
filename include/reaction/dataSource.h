// include/reaction/DATASOURCE.h
#ifndef REACTION_DATASOURCE_H
#define REACTION_DATASOURCE_H

#include "reaction/invalidStrategy.h"
#include "reaction/expression.h"
#include <atomic>

namespace reaction
{
    template <typename TriggerPolicy, typename InvalidStrategy>
    class ActionSource;

    template <typename... Args>
    struct SourceTraits
    {
        using ExprType = Expression<Args...>;
    };

    template <typename TriggerPolicy, typename InvalidStrategy, typename Type>
    struct SourceTraits<DataSource<TriggerPolicy, InvalidStrategy, Type>>
    {
        using ExprType = Expression<TriggerPolicy, Type>;
    };

    template <typename TriggerPolicy, typename InvalidStrategy, typename Type, typename... Args>
    struct SourceTraits<DataSource<TriggerPolicy, InvalidStrategy, Type, Args...>>
    {
        using ExprType = Expression<TriggerPolicy, Type, Args...>;
    };

    template <typename TriggerPolicy, typename InvalidStrategy>
    struct SourceTraits<ActionSource<TriggerPolicy, InvalidStrategy>>
    {
        using ExprType = Expression<TriggerPolicy, void>;
    };

    template <typename Derived, typename InvalidStrategy>
    class Source : public SourceTraits<Derived>::ExprType, public InvalidStrategy
    {
    public:
        using SourceTraits<Derived>::ExprType::Expression;

        template <typename T, typename... A>
        bool set(T &&t, A &&...args)
        {
            return this->setSource(std::forward<T>(t), std::forward<A>(args)...);
        }

        void close()
        {
            ObserverGraph::getInstance().closeNode(this->getShared());
        }

    private:
        void addWeakRef()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_weakRefCount++;
        }

        void releaseWeakRef()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (--m_weakRefCount == 0)
            {
                if constexpr (std::is_same_v<InvalidStrategy, UseLastValidValueStrategy>)
                {
                    this->handleInvalid(static_cast<Derived *>(this));
                }
                else
                {
                    this->handleInvalid(this->getShared());
                }
            }
        }

        template <typename T>
        friend class DataWeakRef;

        std::atomic<int> m_weakRefCount{0};
        std::mutex m_mutex;
    };

    template <typename TriggerPolicy, typename InvalidStrategy, typename Type, typename... Args>
    class DataSource : public Source<DataSource<TriggerPolicy, InvalidStrategy, Type, Args...>, InvalidStrategy>
    {
    public:
        using Source<DataSource<TriggerPolicy, InvalidStrategy, Type, Args...>, InvalidStrategy>::Source;
        using InvStrategy = InvalidStrategy;

        template <typename T>
            requires(!std::is_const_v<Type>)
        DataSource &operator=(T &&t)
        {
            this->updateValue(std::forward<T>(t));
            this->notifyObservers(get() != t);
            return *this;
        }

        auto get()
        {
            return this->getValue();
        }

        auto getUpdate()
        {
            return this->evaluate();
        }
    };

    template <typename TriggerPolicy, typename InvalidStrategy>
    class ActionSource : public Source<ActionSource<TriggerPolicy, InvalidStrategy>, InvalidStrategy>
    {
    public:
        using Source<ActionSource<TriggerPolicy, InvalidStrategy>, InvalidStrategy>::Source;
        using InvStrategy = InvalidStrategy;
    };

    template <typename TriggerPolicy, typename InvalidStrategy, typename Type>
    class FieldSource : public DataSource<TriggerPolicy, InvalidStrategy, Type>
    {
    public:
    };

    template <typename DataType>
    class DataWeakRef
    {
    public:
        using InvStrategy = typename DataType::InvStrategy;

        DataWeakRef(std::shared_ptr<DataType> ptr) : m_weakPtr(ptr)
        {
            if (auto p = m_weakPtr.lock())
                p->addWeakRef();
        }

        ~DataWeakRef()
        {
            if (auto p = m_weakPtr.lock())
                p->releaseWeakRef();
        }

        DataWeakRef(const DataWeakRef &other) : m_weakPtr(other.m_weakPtr)
        {
            if (auto p = m_weakPtr.lock())
                p->addWeakRef();
        }

        DataWeakRef(DataWeakRef &&other) noexcept : m_weakPtr(std::move(other.m_weakPtr)) {}

        DataWeakRef &operator=(const DataWeakRef &other) noexcept
        {
            if (this != &other)
            {
                if (auto p = m_weakPtr.lock())
                    p->releaseWeakRef();
                m_weakPtr = other.m_weakPtr;
                if (auto p = m_weakPtr.lock())
                    p->addWeakRef();
            }
            return *this;
        }

        DataWeakRef &operator=(DataWeakRef &&other) noexcept
        {
            if (this != &other)
            {
                if (auto p = m_weakPtr.lock())
                    p->releaseWeakRef();
                m_weakPtr = std::move(other.m_weakPtr);
            }
            return *this;
        }

        std::shared_ptr<DataType> operator->()
        {
            if (m_weakPtr.expired())
                throw std::runtime_error("Null weak pointer");
            return m_weakPtr.lock();
        }

        DataType &operator*()
        {
            return *this->operator->();
        }

        explicit operator bool() const
        {
            return !m_weakPtr.expired();
        }

    private:
        std::weak_ptr<DataType> m_weakPtr;
    };

    template <typename T>
    struct ExtractDataWeakRef
    {
        using Type = T;
    };

    template <typename T>
    struct ExtractDataWeakRef<DataWeakRef<T>>
    {
        using Type = T;
    };

    template <typename SourceType, typename TriggerPolicy = AlwaysTrigger, typename InvalidStrategy = DirectFailureStrategy>
    using Field = DataWeakRef<FieldSource<TriggerPolicy, InvalidStrategy, SourceType>>;

    template <typename TriggerPolicy = AlwaysTrigger, typename InvalidStrategy = DirectFailureStrategy, typename MetaTypePtr, typename SourceType>
    auto makeFiledSource(MetaTypePtr ptr, SourceType &&value)
    {
        auto ds = std::make_shared<DataSource<TriggerPolicy, InvalidStrategy, std::decay_t<SourceType>>>(std::forward<SourceType>(value));

        return DataWeakRef{ds};
    }

    template <typename TriggerPolicy = AlwaysTrigger, typename InvalidStrategy = DirectFailureStrategy, typename SourceType>
    auto makeConstantDataSource(SourceType &&value)
    {
        auto ptr = std::make_shared<DataSource<TriggerPolicy, InvalidStrategy, const std::decay_t<SourceType>>>(std::forward<SourceType>(value));
        ObserverGraph::getInstance().addNode(ptr);
        return DataWeakRef{ptr};
    }

    template <typename TriggerPolicy = AlwaysTrigger, typename InvalidStrategy = DirectFailureStrategy, typename SourceType>
        requires requires(const SourceType &a, const SourceType &b) { { a == b } -> std::convertible_to<bool>; }
    auto makeMetaDataSource(SourceType &&value)
    {
        auto ptr = std::make_shared<DataSource<TriggerPolicy, InvalidStrategy, std::decay_t<SourceType>>>(std::forward<SourceType>(value));
        // 这里可以通过ptr获取到Data对象的地址，应该是每个fieldSource可以拿到metaSource的节点，然后通过ObserverGraph获取到观察者列表触发
        // 在fieldSource的构造函数
        ObserverGraph::getInstance().addNode(ptr);
        return DataWeakRef{ptr};
    }

    template <typename TriggerPolicy = AlwaysTrigger, typename InvalidStrategy = DirectFailureStrategy, typename Fun, typename... Args>
        requires(sizeof...(Args) > 0)
    auto makeVariableDataSource(Fun &&fun, Args &&...args)
    {
        auto ptr = std::make_shared<DataSource<TriggerPolicy, InvalidStrategy, Fun, typename ExtractDataWeakRef<std::decay_t<Args>>::Type...>>();
        ObserverGraph::getInstance().addNode(ptr);
        ptr->set(std::forward<Fun>(fun), std::forward<Args>(args)...);
        return DataWeakRef{ptr};
    }

    template <typename TriggerPolicy = AlwaysTrigger, typename InvalidStrategy = DirectFailureStrategy, typename Fun, typename... Args>
    auto makeActionSource(Fun &&fun, Args &&...args)
    {
        auto ptr = std::make_shared<ActionSource<TriggerPolicy, InvalidStrategy>>();
        ObserverGraph::getInstance().addNode(ptr);
        ptr->set(std::forward<Fun>(fun), std::forward<Args>(args)...);
        return DataWeakRef{ptr};
    }

} // namespace reaction

#endif // REACTION_DATASOURCE_H