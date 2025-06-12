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

/**
 * @brief Global thread-local registration callback.
 * Used internally to track dependent nodes during expression evaluation.
 */
inline thread_local std::function<void(NodePtr)> g_reg_fun = nullptr;

/**
 * @brief Guard object for setting and resetting the registration callback.
 * Used to track dependency graph nodes during expression construction.
 */
struct RegGuard {
    explicit RegGuard(const std::function<void(NodePtr)> &f) {
        g_reg_fun = f;
    }

    ~RegGuard() {
        g_reg_fun = nullptr;
    }
};

/**
 * @brief Internal implementation of the React wrapper.
 *
 * This class combines expression logic, invalidation strategy, and trigger mode handling.
 *
 * @tparam Expr Expression type.
 * @tparam Type Result value type.
 * @tparam IS   Invalidation strategy.
 * @tparam TM   Triggering mode.
 */
template <typename Expr, typename Type, IsInvaStra IS, IsTrigMode TM>
class ReactImpl : public Expression<Expr, Type, TM>, public IS {
public:
    using Expression<Expr, Type, TM>::Expression;

    /**
     * @brief Assign a new value directly to the reactive variable.
     * Only valid for VarExpr + non-const Type.
     */
    template <typename T>
    void operator=(T &&t) {
        value(std::forward<T>(t));
    }

    /// @brief Returns the current evaluated value.
    decltype(auto) get() const {
        return this->getValue();
    }

    /// @brief Returns raw pointer to the stored object (for pointer-based types).
    auto getRaw() const {
        return this->getRawPtr();
    }

    /**
     * @brief Sets a new expression source and dependencies.
     *
     * @param f    Expression function/lambda.
     * @param args Additional arguments (usually reactive inputs).
     */
    template <typename F, HasArguments... A>
    void set(F &&f, A &&...args) {
        this->setSource(std::forward<F>(f), std::forward<A>(args)...);
        this->notify();
    }

    /**
     * @brief Overload of set with only function input. Dependency is auto-tracked.
     */
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

    /// @brief Set a no-argument expression and auto-track dependencies.
    void set() {
        RegGuard g{[this](NodePtr node) {
            this->addObCb(node);
        }};
        this->setOpExpr();
    }

    /**
     * @brief Sets the actual value directly (only if convertible).
     */
    template <typename T>
        requires(Convertable<T, Type> && IsVarExpr<Expr> && !ConstType<Type>)
    void value(T &&t) {
        this->setValue(std::forward<T>(t));
    }

    /**
     * @brief Closes this reactive node and cleans up any related state.
     */
    void close() {
        ObserverGraph::getInstance().closeNode(this->shared_from_this());
        if constexpr (HasField<Type>) {
            FieldGraph::getInstance().deleteObj(this->getValue().getId());
        }
    }

    /// @brief Increases internal weak reference count.
    void addWeakRef() {
        m_weakRefCount++;
    }

    /// @brief Decreases weak reference count and invalidates if reaching zero.
    void releaseWeakRef() {
        if (--m_weakRefCount == 0) {
            this->handleInvalid(*this);
        }
    }

private:
    std::atomic<int> m_weakRefCount{0}; ///< Reference counter for weak lifetime tracking.
};

/**
 * @brief Public user-facing wrapper around the reactive node implementation.
 *
 * This class manages ownership, lifecycle, and convenient access to reactive values.
 *
 * @tparam Expr Expression type.
 * @tparam Type Final value type.
 * @tparam IS   Invalidation strategy.
 * @tparam TM   Trigger mode type.
 */
template <typename Expr, typename Type, IsInvaStra IS, IsTrigMode TM>
class React {
public:
    using ValueType = Type;
    using ReactType = ReactImpl<Expr, Type, IS, TM>;

    /// @brief Construct a React from shared pointer (usually internal use).
    explicit React(std::shared_ptr<ReactType> ptr = nullptr) : m_weakPtr(ptr) {
        if (auto p = m_weakPtr.lock())
            p->addWeakRef();
    }

    /// @brief Destructor releases weak reference if still alive.
    ~React() {
        if (auto p = m_weakPtr.lock())
            p->releaseWeakRef();
    }

    /// @brief Copy constructor.
    React(const React &other) : m_weakPtr(other.m_weakPtr) {
        if (auto p = m_weakPtr.lock())
            p->addWeakRef();
    }

    /// @brief Move constructor.
    React(React &&other) noexcept : m_weakPtr(std::move(other.m_weakPtr)) {
        other.m_weakPtr.reset();
    }

    /// @brief Copy assignment with reference count handling.
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

    /// @brief Move assignment.
    React &operator=(React &&other) noexcept {
        if (this != &other) {
            if (auto p = m_weakPtr.lock())
                p->releaseWeakRef();
            m_weakPtr = std::move(other.m_weakPtr);
            other.m_weakPtr.reset();
        }
        return *this;
    }

    /// @brief Compare by internal pointer name (used in containers).
    bool operator<(const React &other) const {
        return getName() < other.getName();
    }

    /// @brief Check equality by underlying object pointer.
    bool operator==(const React &other) const {
        return getPtr().get() == other.getPtr().get();
    }

    /// @brief Pointer-like access to raw value.
    auto operator->() const {
        return getPtr()->getRaw();
    }

    /// @brief Checks if this React is valid (non-null).
    explicit operator bool() const {
        return !m_weakPtr.expired();
    }

    /**
     * @brief Value access operator.
     * If within a dependency registration scope, registers node.
     */
    decltype(auto) operator()() const {
        if (g_reg_fun) {
            std::invoke(g_reg_fun, getPtr());
        }
        return get();
    }

    /// @brief Returns the current value.
    decltype(auto) get() const {
        return getPtr()->get();
    }

    /// @brief Reset the expression with new source and dependencies.
    template <typename F, typename... A>
    React &reset(F &&f, A &&...args) {
        getPtr()->set(std::forward<F>(f), std::forward<A>(args)...);
        return *this;
    }

    /// @brief Assigns a value (only if valid).
    template <typename T>
    React &value(T &&t) {
        getPtr()->value(std::forward<T>(t));
        return *this;
    }

    /// @brief Apply a filter to this React node.
    template <typename F, typename... A>
    React &filter(F &&f, A &&...args) {
        getPtr()->filter(std::forward<F>(f), std::forward<A>(args)...);
        return *this;
    }

    /// @brief Closes this reactive node.
    React &close() {
        getPtr()->close();
        return *this;
    }

    /// @brief Assign a human-readable name for debugging/tracing.
    React &setName(const std::string &name) {
        ObserverGraph::getInstance().setName(getPtr(), name);
        return *this;
    }

    /// @brief Get the name assigned to this node.
    std::string getName() const {
        return ObserverGraph::getInstance().getName(getPtr());
    }

private:
    /// @brief Lock the weak pointer and get the shared instance. Throws on failure.
    std::shared_ptr<ReactType> getPtr() const {
        if (m_weakPtr.expired()) {
            throw std::runtime_error("Null weak pointer access");
        }
        return m_weakPtr.lock();
    }

    /// @brief Returns the underlying weak pointer (checked).
    std::weak_ptr<ReactType> getWeak() const {
        if (m_weakPtr.expired()) {
            throw std::runtime_error("Null weak pointer access");
        }
        return m_weakPtr;
    }

    std::weak_ptr<ReactType> m_weakPtr; ///< Weak reference to the implementation node.

    template <typename T, IsTrigMode M>
    friend class CalcExprBase;

    friend struct FilterTrig;
    friend struct std::hash<React<Expr, Type, IS, TM>>;
};

} // namespace reaction

/// @brief Hash support for React to allow use in unordered containers.
namespace std {

using namespace reaction;
template <typename Expr, typename Type, IsInvaStra IS, IsTrigMode TM>
struct hash<React<Expr, Type, IS, TM>> {
    std::size_t operator()(const React<Expr, Type, IS, TM> &react) const noexcept {
        return std::hash<ObserverNode *>{}(react.getPtr().get());
    }
};

} // namespace std
