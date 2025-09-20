/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

/**
 * @file expression.h
 * @brief Unified entry point for reactive expression system
 *
 * This file serves as the main interface for the expression template system,
 * providing all necessary components for creating and working with reactive
 * expressions.
 *
 * The expression system is organized into modular components:
 * - expression/operators.h: All operator functors (AddOp, MulOp, etc.)
 * - expression/expression_types.h: Core expression classes (BinaryOpExpr, UnaryOpExpr, etc.)
 * - expression/expression_builders.h: Helper functions and operator overloads
 *
 * This file contains expression specializations and the core reactive computation logic.
 */

#include <functional>
#include <optional>
#include <vector>
#include "reaction/expression/operators.h"
#include "reaction/expression/expression_types.h"
#include "reaction/expression/expression_builders.h"
#include "reaction/core/concept.h"
#include "reaction/core/resource.h"
#include "reaction/policy/trigger.h"
#include "reaction/core/exception.h"
#include "reaction/graph/observer_graph.h"
#include "reaction/graph/field_graph.h"
#include "reaction/concurrency/global_state.h"
#include "reaction/core/raii_guards.h"

namespace reaction {

/**
 * @brief Marker type for variable expressions.
 */
struct VarExpr {};

/**
 * @brief Marker type for calculated expressions.
 */
struct CalcExpr {};

// === Forward Declarations ===
template <typename Expr, typename Type, IsTrigger TR> class Expression;

/**
 * @brief Core reactive computation logic shared by all calculated expressions.
 *
 * Handles dependency registration, invalidation, and value recomputation.
 *
 * @tparam Type Computed value type.
 * @tparam TR   Triggering mode.
 */
template <typename Type, IsTrigger TR>
class CalcExprBase : public Resource<Type>, public TR {
public:
    /**
     * @brief Sets the function source and its dependencies transactionally.
     *
     * This method ensures atomicity: either all changes succeed,
     * or the node is restored to its original consistent state.
     */
    template <typename F, typename... A>
    void setSource(F &&f, A &&...args) {
        if constexpr (std::convertible_to<ReturnType<F, A...>, Type>) {
            // Check if node is involved in any active batch operations
            auto shared_this = std::static_pointer_cast<CalcExprBase<Type, TR>>(this->shared_from_this());
            if (ObserverGraph::getInstance().isNodeInActiveBatch(shared_this)) {
                REACTION_THROW_BATCH_CONFLICT("Reset operations must be performed outside of batch contexts");
            }

            // Step 1: Save current state for rollback
            auto originalFun = m_fun;
            auto originalValue = [this]() -> std::optional<Type> {
                if constexpr (!VoidType<Type>) {
                    // Safely get value without throwing if not initialized
                    try {
                        return this->getValue();
                    } catch (const std::runtime_error &) {
                        return std::nullopt;
                    }
                } else {
                    return std::nullopt;
                }
            }();

            // Step 2: Get rollback function for observer state
            auto observerRollback = ObserverGraph::getInstance().saveNodeStateForRollback(shared_this);

            try {
                // Step 3: Update observers transactionally
                this->updateObservers(args.getPtr()...);

                // Step 4: Set new function
                m_fun = createFun(std::forward<F>(f), std::forward<A>(args)...);

                // Step 5: Evaluate and update value
                if constexpr (!VoidType<Type>) {
                    this->updateValue(evaluate());
                } else {
                    evaluate();
                }

            } catch (const std::exception &) {
                // Step 6: Rollback on failure
                m_fun = std::move(originalFun);

                // Restore original value if it exists
                if constexpr (!VoidType<Type>) {
                    if (originalValue.has_value()) {
                        this->updateValue(*originalValue);
                    }
                }

                // Restore original observer state
                observerRollback();

                throw; // Re-throw the original exception
            }
        } else {
            REACTION_THROW_TYPE_MISMATCH(typeid(Type).name(), typeid(ReturnType<F, A...>).name());
        }
    }

    /// @brief Registers an observer for dependency tracking.
    void addObCb(const NodePtr &node) {
        this->addOneObserver(node);
    }

    /// @brief Handles value change notifications and trigger checks.
    void valueChanged(bool changed) override {
        handleChange<true>(changed);
    }

    /// @brief Handles value change without notifications.
    void changedNoNotify(bool changed) override {
        handleChange<false>(changed);
    }

private:
    /**
     * @brief Captures and wraps a function with weak references to arguments.
     */
    // TODO: Use move only function return
    // Implementation plan:
    // - Replace std::function with move-only function wrapper
    // - Reduce heap allocations for better performance
    // - Consider using std::unique_function when available
    template <typename F, typename... A>
    auto createFun(F &&f, A &&...args) {
        return [f = std::forward<F>(f), ... args = args.getPtr()]() {
            if constexpr (VoidType<Type>) {
                std::invoke(f, args->get()...);
                return Void{};
            } else {
                return std::invoke(f, args->get()...);
            }
        };
    }

    template <bool Notify>
    void handleChange(bool changed) {
        if constexpr (IsChangeTrig<TR>) {
            this->setChanged(changed);
        }

        if (TR::checkTrig()) {
            bool change = true;
            if constexpr (!VoidType<Type>) {
                change = this->updateValue(evaluate());
            } else {
                evaluate();
            }
            if constexpr (Notify) {
                this->notify(change);
            }
        }
    }

    /// @brief Evaluates the current expression.
    auto evaluate() const {
        if constexpr (VoidType<Type>) {
            std::invoke(m_fun);
        } else {
            return std::invoke(m_fun);
        }
    }

    std::function<Type()> m_fun;
};

/**
 * @brief Primary expression template base.
 *
 * Specialized by expression type and content.
 */
template <typename Expr, typename Type, IsTrigger TR>
class Expression : public CalcExprBase<Type, TR> {
};

/**
 * @brief Expression specialization for reactive variables.
 *
 * Allows manual setting and change detection.
 */
template <typename Type, IsTrigger TR>
class Expression<VarExpr, Type, TR> : public Resource<Type> {
public:
    using Resource<Type>::Resource;

    template <typename T>
    void setValue(T &&t) {
        bool changed = this->updateValue(std::forward<T>(t));
        if (!g_batch_execute) {
            this->notify(changed);
        }
    }
};

/**
 * @brief Expression specialization for binary expressions.
 */
template <typename Op, typename L, typename R, IsTrigger TR>
class Expression<CalcExpr, BinaryOpExpr<Op, L, R>, TR>
    : public CalcExprBase<typename BinaryOpExpr<Op, L, R>::value_type, TR> {
public:
    template <typename T>
        requires(!std::is_same_v<std::remove_cvref_t<T>, Expression>)
    Expression(T &&expr) : m_expr(std::forward<T>(expr)) {
    }

protected:
    /// @brief Sets the operator expression as the current source.
    void setOpExpr() {
        this->setSource([this]() {
            return m_expr();
        });
    }

private:
    BinaryOpExpr<Op, L, R> m_expr;
};

/**
 * @brief Expression specialization for unary expressions.
 */
template <typename Op, typename T, IsTrigger TR>
class Expression<CalcExpr, UnaryOpExpr<Op, T>, TR>
    : public CalcExprBase<typename UnaryOpExpr<Op, T>::value_type, TR> {
public:
    template <typename U>
        requires(!std::is_same_v<std::remove_cvref_t<U>, Expression>)
    Expression(U &&expr) : m_expr(std::forward<U>(expr)) {
    }

protected:
    /// @brief Sets the unary operator expression as the current source.
    void setOpExpr() {
        this->setSource([this]() {
            return m_expr();
        });
    }

private:
    UnaryOpExpr<Op, T> m_expr;
};

} // namespace reaction
