#include "reaction/expression.h"
#include <atomic>

namespace reaction
{
    template <typename Type, typename... Args>
    class ReactImpl : public Expression<Type, Args...>
    {
    public:
        using ValueType = Expression<Type, Args...>::ValueType;
        using ExprType = Expression<Type, Args...>::ExprType;
        using Expression<Type, Args...>::Expression;
        auto get() const
        {
            return this->getValue();
        }

        template <typename T>
            requires(Convertable<T, ValueType> && IsVarExpr<ExprType>)
        void value(T &&t)
        {
            this->updateValue(std::forward<T>(t));
            this->notify();
        }

        void addWeakRef()
        {
            m_weakRefCount++;
        }

        void releaseWeakRef()
        {
            if (--m_weakRefCount == 0)
            {
                ObserverGraph::getInstance().removeNode(this->shared_from_this());
            }
        }

    private:
        std::atomic<int> m_weakRefCount{0};
    };

    template <typename ReactType>
    class React
    {
    public:
        explicit React(std::shared_ptr<ReactType> ptr = nullptr) : m_weakPtr(ptr)
        {
            if (auto p = m_weakPtr.lock())
                p->addWeakRef();
        }

        ~React()
        {
            if (auto p = m_weakPtr.lock())
                p->releaseWeakRef();
        }

        React(const React &other) : m_weakPtr(other.m_weakPtr)
        {
            if (auto p = m_weakPtr.lock())
                p->addWeakRef();
        }

        React(React &&other) noexcept : m_weakPtr(std::move(other.m_weakPtr))
        {
            other.m_weakPtr.reset();
        }

        React &operator=(const React &other) noexcept
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

        React &operator=(React &&other) noexcept
        {
            if (this != &other)
            {
                if (auto p = m_weakPtr.lock())
                    p->releaseWeakRef();
                m_weakPtr = std::move(other.m_weakPtr);
                other.m_weakPtr.reset();
            }
            return *this;
        }

        explicit operator bool() const {
            return !m_weakPtr.expired();
        }

        auto get() const
        {
            return getPtr()->get();
        }

        template <typename T>
        void value(T &&t)
        {
            getPtr()->value(std::forward<T>(t));
        }

        std::shared_ptr<ReactType> getPtr() const
        {
            if (m_weakPtr.expired())
            {
                throw std::runtime_error("Null weak pointer access");
            }
            return m_weakPtr.lock();
        }

        std::weak_ptr<ReactType> m_weakPtr;
    };

    template <typename T>
    auto var(T &&t)
    {
        auto ptr = std::make_shared<ReactImpl<T>>(std::forward<T>(t));
        ObserverGraph::getInstance().addNode(ptr);
        return React(ptr);
    }

    template <typename Fun, typename... Args>
    auto calc(Fun &&fun, Args &&...args)
    {
        auto ptr = std::make_shared<ReactImpl<std::decay_t<Fun>, std::decay_t<Args>...>>(std::forward<Fun>(fun), std::forward<Args>(args)...);
        ObserverGraph::getInstance().addNode(ptr);
        return React(ptr);
    }
}