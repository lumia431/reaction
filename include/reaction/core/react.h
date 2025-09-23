/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/concurrency/global_state.h"
#include "reaction/concurrency/thread_safety.h"
#include "reaction/core/exception.h"
#include "reaction/core/raii_guards.h"
#include "reaction/expression/atomic_operations.h"
#include "reaction/expression/expression.h"
#include "reaction/graph/batch.h"
#include "reaction/policy/invalidation.h"

namespace reaction {

/**
 * @brief Internal implementation of the React wrapper.
 *
 * This class combines expression logic, invalidation strategy, and trigger mode handling.
 *
 * @tparam Expr Expression type.
 * @tparam Type Result value type.
 * @tparam IV   Invalidation strategy.
 * @tparam TR   Triggering mode.
 */
template <typename Expr, typename Type, IsInvalidation IV, IsTrigger TR>
class ReactImpl final : public Expression<Expr, Type, TR>, public IV {
public:
    using Expression<Expr, Type, TR>::Expression;

    /**
     * @brief Assign a new value directly to the reactive variable.
     * Only valid for VarExpr + non-const Type.
     */
    template <typename T>
    void operator=(T &&t) {
        value(std::forward<T>(t));
    }

    /// @brief Returns the current evaluated value.
    [[nodiscard]] decltype(auto) get() const {
        return this->getValue();
    }

    /// @brief Returns raw pointer to the stored object (for pointer-based types).
    [[nodiscard]] auto getRaw() const {
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
            auto g = makeRegFunGuard([this](const NodePtr &node) {
                this->addObCb(node);
            });
            this->setSource(std::forward<F>(f));
        }
        this->notify();
    }

    /// @brief Set a no-argument expression and auto-track dependencies.
    void set() {
        auto g = makeRegFunGuard([this](const NodePtr &node) {
            this->addObCb(node);
        });
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
    void addWeakRef() noexcept {
        m_weakRefCount++;
    }

    /// @brief Decreases weak reference count and invalidates if reaching zero.
    void releaseWeakRef() noexcept {
        if (--m_weakRefCount == 0) {
            this->handleInvalid(*this);
        }
    }

    /**
     * @brief Generic atomic operation helper.
     *
     * @tparam F Operation function type (should take Type& and return bool indicating if changed).
     * @param operation The operation to perform on the value.
     * @param alwaysChanged If true, always consider the operation as changing the value.
     */
    template <typename F>
    void atomicOperation(F &&operation, bool alwaysChanged = false) {
        REACTION_REGISTER_THREAD();
        bool changed = false;
        {
            ConditionalUniqueLock<ConditionalSharedMutex> lock(this->m_valueMutex);
            if (this->m_ptr) {
                changed = operation(*this->m_ptr);
                if (alwaysChanged) {
                    changed = true;
                }
            }
        }
        if (!g_batch_execute && changed) {
            this->notify(true);
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
 * @tparam IV   Invalidation strategy.
 * @tparam TR   Trigger mode type.
 */
template <typename Expr, typename Type, IsInvalidation IV, IsTrigger TR>
class React {
public:
    using value_type = Type;
    using react_type = ReactImpl<Expr, Type, IV, TR>;

    /// @brief Construct a React from shared pointer (usually internal use).
    explicit React(std::shared_ptr<react_type> ptr = nullptr) noexcept : m_weakPtr(ptr) {
        safeAddRef();
    }

    /// @brief Destructor releases weak reference if still alive.
    ~React() {
        safeReleaseRef();
    }

    /// @brief Copy constructor.
    React(const React &other) noexcept : m_weakPtr(other.m_weakPtr) {
        safeAddRef();
    }

    /// @brief Move constructor.
    React(React &&other) noexcept : m_weakPtr(std::move(other.m_weakPtr)) {
        other.m_weakPtr.reset();
    }

    /// @brief Copy assignment with reference count handling.
    React &operator=(const React &other) noexcept {
        if (this != &other) {
            safeReleaseRef();
            m_weakPtr = other.m_weakPtr;
            safeAddRef();
        }
        return *this;
    }

    /// @brief Move assignment.
    React &operator=(React &&other) noexcept {
        if (this != &other) {
            safeReleaseRef();
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
    [[nodiscard]] auto operator->() const {
        return getPtr()->getRaw();
    }

    /// @brief Checks if this React is valid (non-null).
    [[nodiscard]] explicit operator bool() const {
        return !m_weakPtr.expired();
    }

    /**
     * @brief Value access operator.
     * If within a dependency registration scope, registers node.
     */
    [[nodiscard]] decltype(auto) operator()() const {
        if (g_reg_fun) {
            std::invoke(g_reg_fun, getPtr());
        }
        return get();
    }

    /// @brief Returns the current value.
    [[nodiscard]] decltype(auto) get() const {
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
        if (g_batch_fun) {
            std::invoke(g_batch_fun, getPtr());
        } else {
            getPtr()->value(std::forward<T>(t));
        }
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
    [[nodiscard]] std::string getName() const {
        return ObserverGraph::getInstance().getName(getPtr());
    }

    // ==================== Compound Assignment Operators ====================

    /// @brief Compound addition assignment operator (+=)
    template <typename U>
        requires(IsVarExpr<Expr> && !ConstType<Type> && AddAssignable<Type, U>)
    React &operator+=(const U &rhs) {
        atomicAddAssign(*getPtr(), rhs);
        return *this;
    }

    /// @brief Compound subtraction assignment operator (-=)
    template <typename U>
        requires(IsVarExpr<Expr> && !ConstType<Type> && SubtractAssignable<Type, U>)
    React &operator-=(const U &rhs) {
        atomicSubtractAssign(*getPtr(), rhs);
        return *this;
    }

    /// @brief Compound multiplication assignment operator (*=)
    template <typename U>
        requires(IsVarExpr<Expr> && !ConstType<Type> && MultiplyAssignable<Type, U>)
    React &operator*=(const U &rhs) {
        atomicMultiplyAssign(*getPtr(), rhs);
        return *this;
    }

    /// @brief Compound division assignment operator (/=)
    template <typename U>
        requires(IsVarExpr<Expr> && !ConstType<Type> && DivideAssignable<Type, U>)
    React &operator/=(const U &rhs) {
        atomicDivideAssign(*getPtr(), rhs);
        return *this;
    }

    /// @brief Compound modulo assignment operator (%=)
    template <typename U>
        requires(IsVarExpr<Expr> && !ConstType<Type> && ModuloAssignable<Type, U>)
    React &operator%=(const U &rhs) {
        atomicModuloAssign(*getPtr(), rhs);
        return *this;
    }

    /// @brief Compound bitwise AND assignment operator (&=)
    template <typename U>
        requires(IsVarExpr<Expr> && !ConstType<Type> && BitwiseAndAssignable<Type, U>)
    React &operator&=(const U &rhs) {
        atomicBitwiseAndAssign(*getPtr(), rhs);
        return *this;
    }

    /// @brief Compound bitwise OR assignment operator (|=)
    template <typename U>
        requires(IsVarExpr<Expr> && !ConstType<Type> && BitwiseOrAssignable<Type, U>)
    React &operator|=(const U &rhs) {
        atomicBitwiseOrAssign(*getPtr(), rhs);
        return *this;
    }

    /// @brief Compound bitwise XOR assignment operator (^=)
    template <typename U>
        requires(IsVarExpr<Expr> && !ConstType<Type> && BitwiseXorAssignable<Type, U>)
    React &operator^=(const U &rhs) {
        atomicBitwiseXorAssign(*getPtr(), rhs);
        return *this;
    }

    /// @brief Compound left shift assignment operator (<<=)
    template <typename U>
        requires(IsVarExpr<Expr> && !ConstType<Type> && LeftShiftAssignable<Type, U>)
    React &operator<<=(const U &rhs) {
        atomicLeftShiftAssign(*getPtr(), rhs);
        return *this;
    }

    /// @brief Compound right shift assignment operator (>>=)
    template <typename U>
        requires(IsVarExpr<Expr> && !ConstType<Type> && RightShiftAssignable<Type, U>)
    React &operator>>=(const U &rhs) {
        atomicRightShiftAssign(*getPtr(), rhs);
        return *this;
    }

    // ==================== Increment/Decrement Operators ====================

    /// @brief Pre-increment operator (++var)
    template <typename T = Type>
        requires(IsVarExpr<Expr> && !ConstType<Type> && PreIncrementable<T>)
    React &operator++() {
        atomicIncrement(*getPtr());
        return *this;
    }

    /// @brief Post-increment operator (var++)
    template <typename T = Type>
        requires(IsVarExpr<Expr> && !ConstType<Type> && PostIncrementable<T>)
    Type operator++(int) {
        return atomicPostIncrement(*getPtr());
    }

    /// @brief Pre-decrement operator (--var)
    template <typename T = Type>
        requires(IsVarExpr<Expr> && !ConstType<Type> && PreDecrementable<T>)
    React &operator--() {
        atomicDecrement(*getPtr());
        return *this;
    }

    /// @brief Post-decrement operator (var--)
    template <typename T = Type>
        requires(IsVarExpr<Expr> && !ConstType<Type> && PostDecrementable<T>)
    Type operator--(int) {
        return atomicPostDecrement(*getPtr());
    }

private:
    /// @brief Get the internal shared pointer for advanced operations.
    [[nodiscard]] std::shared_ptr<react_type> getPtr() const {
        // Thread-safe access to weak_ptr using conditional mutex
        ConditionalSharedLock<ConditionalSharedMutex> lock(m_ptrMutex);
        auto ptr = m_weakPtr.lock();
        if (!ptr) [[unlikely]] {
            REACTION_THROW_NULL_POINTER("weak pointer lock failed");
        }

        return ptr;
    }

    /**
     * @brief Safely increment the weak reference count.
     *
     * Only increments if the weak pointer can be locked successfully.
     */
    void safeAddRef() noexcept {
        ConditionalSharedLock<ConditionalSharedMutex> lock(m_ptrMutex);
        if (auto p = m_weakPtr.lock()) [[likely]]
            p->addWeakRef();
    }

    /**
     * @brief Safely decrement the weak reference count.
     *
     * Only decrements if the weak pointer can be locked successfully.
     */
    void safeReleaseRef() noexcept {
        ConditionalSharedLock<ConditionalSharedMutex> lock(m_ptrMutex);
        if (auto p = m_weakPtr.lock()) [[likely]]
            p->releaseWeakRef();
    }

    std::weak_ptr<react_type> m_weakPtr;       ///< Weak reference to the implementation node.
    mutable ConditionalSharedMutex m_ptrMutex; ///< Mutex for thread-safe weak_ptr access.

    template <typename T, IsTrigger M>
    friend class CalcExprBase;

    friend struct FilterTrig;
    friend struct std::hash<React<Expr, Type, IV, TR>>;
};

} // namespace reaction

/// @brief Hash support for React to allow use in unordered containers.
namespace std {

using namespace reaction;
template <typename Expr, typename Type, IsInvalidation IV, IsTrigger TR>
struct hash<React<Expr, Type, IV, TR>> {
    std::size_t operator()(const React<Expr, Type, IV, TR> &react) const noexcept {
        return std::hash<ObserverNode *>{}(react.getPtr().get());
    }
};

} // namespace std
