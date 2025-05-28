#include "reaction/expression.h"

namespace reaction {
inline thread_local std::function<void(NodePtr)> g_reg_fun = nullptr;

struct RegGuard {
    RegGuard(const std::function<void(NodePtr)> &f) {
        g_reg_fun = f;
    }
    ~RegGuard() {
        g_reg_fun = nullptr;
    }
};

template <typename Type, typename... Args>
class ReactImpl : public Expression<Type, Args...> {
public:
    using ValueType = Expression<Type, Args...>::ValueType;
    using ExprType = Expression<Type, Args...>::ExprType;
    using Expression<Type, Args...>::Expression;

    template <typename T>
    void operator=(T &&t) {
        value(std::forward<T>(t));
    }

    decltype(auto) get() const {
        return this->getValue();
    }

    auto getRaw() const {
        return this->getRawPtr();
    }

    template <typename F, HasArguments... A>
    ReactionError set(F &&f, A &&...args) {
        return this->setSource(std::forward<F>(f), std::forward<A>(args)...);
    }

    template <typename F>
    ReactionError set(F &&f) {
        RegGuard g{[this](NodePtr node) {
            this->addObCb(node);
        }};
        return this->setSource(std::forward<F>(f));
    }

    ReactionError set() {
        RegGuard g{[this](NodePtr node) {
            this->addObCb(node);
        }};
        return this->setOpExpr();
    }

    template <typename T>
    requires(Convertable<T, ValueType> &&IsVarExpr<ExprType> && !ConstType<ValueType>) void value(T &&t) {
        this->updateValue(std::forward<T>(t));
        this->notify();
    }

    void addWeakRef() {
        m_weakRefCount++;
    }

    void releaseWeakRef() {
        if (--m_weakRefCount == 0) {
            ObserverGraph::getInstance().closeNode(this->shared_from_this());
            if constexpr (HasField<ValueType>) {
                FieldGraph::getInstance().deleteObj(this->getValue().getId());
            }
        }
    }

private:
    std::atomic<int> m_weakRefCount{0};
};

template <typename ReactType>
class React {
public:
    using ValueType = typename ReactType::ValueType;
    explicit React(std::shared_ptr<ReactType> ptr = nullptr) : m_weakPtr(ptr) {
        if (auto p = m_weakPtr.lock())
            p->addWeakRef();
    }

    ~React() {
        if (auto p = m_weakPtr.lock())
            p->releaseWeakRef();
    }

    React(const React &other) : m_weakPtr(other.m_weakPtr) {
        if (auto p = m_weakPtr.lock())
            p->addWeakRef();
    }

    React(React &&other) noexcept : m_weakPtr(std::move(other.m_weakPtr)) {
        other.m_weakPtr.reset();
    }

    React &operator=(const React &other) noexcept {
        if (this != &other) {
            if (auto p = m_weakPtr.lock())
                p->releaseWeakRef();
            m_weakPtr = other.m_weakPtr;
            if (auto p = m_weakPtr.lock())
                p->addWeakRef();
        }
        return *this;
    }

    React &operator=(React &&other) noexcept {
        if (this != &other) {
            if (auto p = m_weakPtr.lock())
                p->releaseWeakRef();
            m_weakPtr = std::move(other.m_weakPtr);
            other.m_weakPtr.reset();
        }
        return *this;
    }

    auto operator->() const {
        return getPtr()->getRaw();
    }

    ReactType &operator*() const {
        return *getPtr();
    }

    explicit operator bool() const {
        return !m_weakPtr.expired();
    }

    decltype(auto) operator()() const {
        if (g_reg_fun) {
            std::invoke(g_reg_fun, getPtr());
        }
        return get();
    }

    decltype(auto) get() const {
        return getPtr()->get();
    }

    template <typename F, typename... A>
    ReactionError reset(F &&f, A &&...args) {
        return getPtr()->set(std::forward<F>(f), std::forward<A>(args)...);
    }

    template <typename T>
    React &value(T &&t) {
        getPtr()->value(std::forward<T>(t));
        return *this;
    }

    React &setName(const std::string &name) {
        ObserverGraph::getInstance().setName(getPtr(), name);
        return *this;
    }

    std::string getName() const {
        return ObserverGraph::getInstance().getName(getPtr());
    }

    std::shared_ptr<ReactType> getPtr() const {
        if (m_weakPtr.expired()) {
            throw std::runtime_error("Null weak pointer access");
        }
        return m_weakPtr.lock();
    }

    std::weak_ptr<ReactType> m_weakPtr;
};

template <typename SrcType>
using Field = React<ReactImpl<std::decay_t<SrcType>>>;

class FieldBase {
public:
    template <typename T>
    auto field(T &&t) {
        auto ptr = std::make_shared<ReactImpl<std::decay_t<T>>>(std::forward<T>(t));
        ObserverGraph::getInstance().addNode(ptr->shared_from_this());
        FieldGraph::getInstance().addObj(m_id, ptr->shared_from_this());
        return React(ptr);
    }

    u_int64_t getId() {
        return m_id;
    }

private:
    UniqueID m_id;
};

template <typename SrcType>
auto constVar(SrcType &&t) {
    auto ptr = std::make_shared<ReactImpl<const std::decay_t<SrcType>>>(std::forward<SrcType>(t));
    ObserverGraph::getInstance().addNode(ptr);
    return React(ptr);
}

template <typename SrcType>
auto var(SrcType &&t) {
    auto ptr = std::make_shared<ReactImpl<std::decay_t<SrcType>>>(std::forward<SrcType>(t));
    ObserverGraph::getInstance().addNode(ptr);
    if constexpr (HasField<SrcType>) {
        FieldGraph::getInstance().bindField(t.getId(), ptr->shared_from_this());
    }
    return React(ptr);
}

template <typename OpExpr>
auto expr(OpExpr &&opExpr) {
    auto ptr = std::make_shared<ReactImpl<std::decay_t<OpExpr>>>(std::forward<OpExpr>(opExpr));
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set();
    return React{ptr};
}

template <typename Fun, typename... Args>
auto calc(Fun &&fun, Args &&...args) {
    auto ptr = std::make_shared<ReactImpl<std::decay_t<Fun>, std::decay_t<Args>...>>();
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set(std::forward<Fun>(fun), std::forward<Args>(args)...);
    return React(ptr);
}

template <typename Fun, typename... Args>
auto action(Fun &&fun, Args &&...args) {
    return calc(std::forward<Fun>(fun), std::forward<Args>(args)...);
}
} // namespace reaction