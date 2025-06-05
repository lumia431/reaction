#pragma once

#include "reaction/expression.h"
#include "reaction/invalidStrategy.h"
#include "reaction/valueTpErase.h"

namespace reaction {

template <typename TM, typename IS, typename Type, typename... Args>
class ReactImpl : public Expression<TM, Type, Args...>, public IS {
public:
    using ValueType = Expression<TM, Type, Args...>::ValueType;
    using ExprType = Expression<TM, Type, Args...>::ExprType;
    using Expression<TM, Type, Args...>::Expression;
    using TrigMode = TM;

    template <typename T>
    void operator=(T &&t) {
        value(std::forward<T>(t));
    }

    decltype(auto) get() const {
        return this->getValue();
    }

    auto getRaw() const {
        if constexpr (!VoidType<ValueType>) {
            return this->getRawPtr();
        }
    }

    template <typename F, HasArguments... A>
    void set(F &&f, A &&...args) {
        this->setSource(std::forward<F>(f), std::forward<A>(args)...);
    }

    template <typename F>
    void set(F &&f) {
        if constexpr (!IsVarExpr<ExprType>) {
            RegGuard g{[this](NodePtr node) {
                this->addObCb(node);
            }};
            this->setSource(std::forward<F>(f));
        }
        this->notify();
    }

    void set() {
        if constexpr (!IsVarExpr<ExprType>) {
            RegGuard g{[this](NodePtr node) {
                this->addObCb(node);
            }};
            this->setOpExpr();
        }
    }

    template <typename T>
    void value(T &&t) {
        if constexpr (!VoidType<ValueType> && !ConstType<ValueType>) {
            this->updateValue(std::forward<T>(t));
        }
        this->notify();
    }

    void close() {
        ObserverGraph::getInstance().closeNode(this->shared_from_this());
        if constexpr (HasField<ValueType>) {
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

template <typename ReactType>
class React : public ReactBase<typename ReactType::ExprType, typename ReactType::ValueType> {
public:
    using ExprType = typename ReactType::ExprType;
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

    explicit operator bool() const override {
        return !m_weakPtr.expired();
    }

    auto operator->() const {
        if constexpr (!VoidType<ValueType>) {
            return getPtr()->getRaw();
        }
        return static_cast<ValueType *>(nullptr);
    }

    decltype(auto) operator()() const {
        if (g_reg_fun) {
            std::invoke(g_reg_fun, getNodePtr());
        }
        return get();
    }

    ValueTpErase getAny() const override {
        return getPtr()->get();
    }

    ValueTpErase reg() const override {
        return operator()();
    }

    ValueTpErase arrow() const override {
        return operator->();
    }

    decltype(auto) get() const {
        return getPtr()->get();
    }

    React &reset(const std::function<ValueTpErase()> &fun) override {
        getPtr()->set(fun);
        return *this;
    }

    template <typename T>
    React &value(T &&t) {
        getPtr()->value(std::forward<T>(t));
        return *this;
    }

    React &value(ValueTpErase val) override {
        getPtr()->value(val.value<ValueType>());
        return *this;
    }

    React &filter(const std::function<bool()> &fun) override {
        if constexpr (std::is_same_v<typename ReactType::TrigMode, FilterTrig>) {
            getPtr()->filter(fun);
        }
        return *this;
    }

    React &close() override {
        getPtr()->close();
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

    NodePtr getNodePtr() const {
        return getPtr()->shared_from_this();
    }

    std::weak_ptr<ReactType> m_weakPtr;

    template <typename TM, typename Fun, typename... Args>
    friend class Expression;
    friend struct FilterTrig;
};
} // namespace reaction