/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/expression.h"
#include "reaction/invalidStrategy.h"

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

template <typename Expr, typename Type, IsInvaStra IS, IsTrigMode TM>
class ReactImpl : public Expression<Expr, Type, TM>, public IS {
public:
    using Expression<Expr, Type, TM>::Expression;

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
    void set(F &&f, A &&...args) {
        this->setSource(std::forward<F>(f), std::forward<A>(args)...);
        this->notify();
    }

    template <typename F>
    void set(F &&f) {
        {
            RegGuard g{[this](NodePtr node) {
                this->addObCb(node);
            }};
            this->setSource(std::forward<F>(f));
        }
        this->notify();
    }

    void set() {
        RegGuard g{[this](NodePtr node) {
            this->addObCb(node);
        }};
        this->setOpExpr();
    }

    template <typename T>
        requires(Convertable<T, Type> && IsVarExpr<Expr> && !ConstType<Type>)
    void value(T &&t) {
        this->setValue(std::forward<T>(t));
    }

    void close() {
        ObserverGraph::getInstance().closeNode(this->shared_from_this());
        if constexpr (HasField<Type>) {
            FieldGraph::getInstance().deleteObj(this->getValue().getId());
        }
    }

    void addWeakRef() {
        m_weakRefCount++;
    }

    void releaseWeakRef() {
        if (--m_weakRefCount == 0) {
            this->handleInvalid(*this);
        }
    }

private:
    std::atomic<int> m_weakRefCount{0};
};

template <typename Expr, typename Type, IsInvaStra IS, IsTrigMode TM>
class React {
public:
    using ValueType = Type;
    using ReactType = ReactImpl<Expr, Type, IS, TM>;
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

    bool operator<(const React &other) const {
        return getName() < other.getName();
    }

    bool operator==(const React &other) const {
        return getPtr().get() == other.getPtr().get();
    }

    auto operator->() const {
        return getPtr()->getRaw();
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
    React &reset(F &&f, A &&...args) {
        getPtr()->set(std::forward<F>(f), std::forward<A>(args)...);
        return *this;
    }

    template <typename T>
    React &value(T &&t) {
        getPtr()->value(std::forward<T>(t));
        return *this;
    }

    template <typename F, typename... A>
    React &filter(F &&f, A &&...args) {
        getPtr()->filter(std::forward<F>(f), std::forward<A>(args)...);
        return *this;
    }

    React &close() {
        getPtr()->close(); // Close the data source
        return *this;
    }

    React &setName(const std::string &name) {
        ObserverGraph::getInstance().setName(getPtr(), name);
        return *this;
    }

    std::string getName() const {
        return ObserverGraph::getInstance().getName(getPtr());
    }

private:
    std::shared_ptr<ReactType> getPtr() const {
        if (m_weakPtr.expired()) {
            throw std::runtime_error("Null weak pointer access");
        }
        return m_weakPtr.lock();
    }

    std::weak_ptr<ReactType> getWeak() const {
        if (m_weakPtr.expired()) {
            throw std::runtime_error("Null weak pointer access");
        }
        return m_weakPtr;
    }

    std::weak_ptr<ReactType> m_weakPtr;
    template <typename T, IsTrigMode M>
    friend class CalcExprBase;
    friend struct FilterTrig;

    friend struct std::hash<React<Expr, Type, IS, TM>>;
};

} // namespace reaction

namespace std {

using namespace reaction;
template <typename Expr, typename Type, IsInvaStra IS, IsTrigMode TM>
struct hash<React<Expr, Type, IS, TM>> {
    std::size_t operator()(const React<Expr, Type, IS, TM> &react) const noexcept {
        return std::hash<ObserverNode *>{}(react.getPtr().get());
    }
};

} // namespace std