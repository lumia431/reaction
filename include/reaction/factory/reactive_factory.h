/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/core/react.h"
#include "reaction/graph/observer_graph.h"
#include "reaction/graph/field_graph.h"
#include "reaction/core/id_generator.h"

namespace reaction {

/**
 * @brief Alias template for a reactive variable (Var) based on VarExpr expression type.
 *
 * @tparam SrcType The underlying data type held by this reactive variable.
 * @tparam IV Invalidation strategy, default is KeepHandle.
 * @tparam TR Trigger mode, default is ChangeTrig.
 */
template <NonReact SrcType, IsInvalidation IV = KeepHandle, IsTrigger TR = ChangeTrig>
using Var = React<VarExpr, SrcType, IV, TR>;

/**
 * @brief Alias template for a reactive expression (Expr) based on BinaryOpExpr expression type.
 *
 * @tparam SrcType The underlying data type produced by this reactive calculation.
 * @tparam IV Invalidation strategy, default is KeepHandle.
 * @tparam TR Trigger mode, default is ChangeTrig.
 */
template <NonReact SrcType, IsInvalidation IV = KeepHandle, IsTrigger TR = ChangeTrig>
using Calc = React<CalcExpr, SrcType, IV, TR>;

/**
 * @brief Alias template for a reactive action (Action) based on CalcExpr expression type.
 *
 * @tparam IV Invalidation strategy, default is KeepHandle.
 * @tparam TR Trigger mode, default is ChangeTrig.
 */
template <IsInvalidation IV = KeepHandle, IsTrigger TR = ChangeTrig>
using Action = React<CalcExpr, Void, IV, TR>;

/**
 * @brief Base class representing a field container in the reactive graph.
 */
class FieldBase {
public:
    /**
     * @brief Create a reactive field from the provided initial value.
     *
     * This function creates a shared pointer to a ReactImpl representing
     * a variable expression holding the given value. It registers the node
     * to both the global ObserverGraph and FieldGraph using this FieldBase's ID.
     *
     * @tparam TR Trigger mode, default is ChangeTrig.
     * @tparam IV Invalidation strategy, default is KeepHandle.
     * @tparam T The type of the initial value (deduced).
     * @param t The initial value to store in the field.
     * @return React<VarExpr, std::remove_cvref_t<T>, IV, TR> A reactive wrapper around the stored value.
     */
    template <IsTrigger TR = ChangeTrig, IsInvalidation IV = KeepHandle, NonReact T>
    auto field(T &&t) {
        auto ptr = std::make_shared<ReactImpl<VarExpr, std::remove_cvref_t<T>, IV, TR>>(std::forward<T>(t));
        ObserverGraph::getInstance().addNode(ptr->shared_from_this());
        FieldGraph::getInstance().addObj(m_id, ptr->shared_from_this());
        return React{ptr};
    }

    /**
     * @brief Get the unique identifier of this field base.
     *
     * @return uint64_t The unique ID.
     */
    uint64_t getId() {
        return m_id;
    }

private:
    UniqueID m_id; ///< Unique identifier for this field instance.
};

/**
 * @brief Create a constant reactive variable wrapping the given value.
 *
 * This is similar to var(), but the wrapped value is const.
 * The reactive node is registered to the ObserverGraph.
 *
 * @tparam TR Trigger mode, default is ChangeTrig.
 * @tparam IV Invalidation strategy, default is KeepHandle.
 * @tparam SrcType The type of the source value.
 * @param t The value to wrap as a const reactive variable.
 * @return React<VarExpr, const std::remove_cvref_t<SrcType>, IV, TR> Reactive constant wrapper.
 */
template <IsTrigger TR = ChangeTrig, IsInvalidation IV = KeepHandle, NonReact SrcType>
auto constVar(SrcType &&t) {
    auto ptr = std::make_shared<ReactImpl<VarExpr, const std::remove_cvref_t<SrcType>, IV, TR>>(std::forward<SrcType>(t));
    ObserverGraph::getInstance().addNode(ptr);
    return React{ptr};
}

/**
 * @brief Create a reactive variable wrapping the given value.
 *
 * If the source type supports the Field concept, binds the field in the FieldGraph.
 * Registers the reactive node to the ObserverGraph.
 *
 * @tparam TR Trigger mode, default is ChangeTrig.
 * @tparam IV Invalidation strategy, default is KeepHandle.
 * @tparam SrcType The type of the source value.
 * @param t The value to wrap reactively.
 * @return React<VarExpr, std::remove_cvref_t<SrcType>, IV, TR> Reactive variable wrapper.
 */
template <IsTrigger TR = ChangeTrig, IsInvalidation IV = KeepHandle, NonReact SrcType>
auto var(SrcType &&t) {
    auto ptr = std::make_shared<ReactImpl<VarExpr, std::remove_cvref_t<SrcType>, IV, TR>>(std::forward<SrcType>(t));
    ObserverGraph::getInstance().addNode(ptr);
    if constexpr (HasField<SrcType>) {
        FieldGraph::getInstance().bindField(t.getId(), ptr->shared_from_this());
    }
    return React{ptr};
}

/**
 * @brief Create a reactive expression wrapping the given operator expression.
 *
 * Registers the node to ObserverGraph, then sets (evaluates) the expression immediately.
 * Supports both binary and unary operator expressions.
 *
 * @tparam TR Trigger mode, default is ChangeTrig.
 * @tparam IV Invalidation strategy, default is KeepHandle.
 * @tparam OpExpr The operator expression type (binary or unary).
 * @param opExpr The operator expression to wrap.
 * @return React<CalcExpr, std::remove_cvref_t<OpExpr>, IV, TR> Reactive calculation wrapper.
 */
template <IsTrigger TR = ChangeTrig, IsInvalidation IV = KeepHandle, IsOpExpr OpExpr>
auto expr(OpExpr &&opExpr) {
    auto ptr = std::make_shared<ReactImpl<CalcExpr, std::remove_cvref_t<OpExpr>, IV, TR>>(std::forward<OpExpr>(opExpr));
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set();
    return React{ptr};
}

/**
 * @brief Create a reactive calculation based on a callable and its arguments.
 *
 * Registers the node to ObserverGraph and immediately sets (evaluates) the calculation.
 *
 * @tparam TR Trigger mode, default is ChangeTrig.
 * @tparam IV Invalidation strategy, default is KeepHandle.
 * @tparam Fun Callable type.
 * @tparam Args Argument types for the callable.
 * @param fun The callable to invoke reactively.
 * @param args Arguments to forward to the callable.
 * @return React<CalcExpr, ReturnType<Fun, Args...>, IV, TR> Reactive calculation wrapper.
 */
template <IsTrigger TR = ChangeTrig, IsInvalidation IV = KeepHandle, typename Fun, typename... Args>
auto calc(Fun &&fun, Args &&...args) {
    auto ptr = std::make_shared<ReactImpl<CalcExpr, ReturnType<Fun, Args...>, IV, TR>>();
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set(std::forward<Fun>(fun), std::forward<Args>(args)...);
    return React{ptr};
}

/**
 * @brief Alias for calc(), used to create a reactive action.
 *
 * @tparam TR Trigger mode, default is ChangeTrig.
 * @tparam IV Invalidation strategy, default is KeepHandle.
 * @tparam Fun Callable type.
 * @tparam Args Argument types.
 * @param fun The callable to invoke reactively.
 * @param args Arguments to forward to the callable.
 * @return The reactive action wrapper.
 */
template <IsTrigger TR = ChangeTrig, IsInvalidation IV = KeepHandle, typename Fun, typename... Args>
auto action(Fun &&fun, Args &&...args) {
    return calc<TR, IV>(std::forward<Fun>(fun), std::forward<Args>(args)...);
}

/**
 * @brief Factory function to create a reactive wrapper from a value or expression.
 *
 * If the input is a binary operator expression, creates an expr().
 * If the input is an invocable callable, creates a calc().
 * Otherwise, creates a var().
 *
 * @tparam T The type of input.
 * @param t The input value, expression, or callable.
 * @return Corresponding reactive wrapper.
 */
template <NonReact T>
auto create(T &&t) {
    if constexpr (IsBinaryOpExpr<std::remove_cvref_t<T>>) {
        return expr(std::forward<T>(t));
    } else if constexpr (InvocableType<T>) {
        return calc(std::forward<T>(t));
    } else {
        return var(std::forward<T>(t));
    }
}

/**
 * @brief Factory function to create a reactive calculation from a callable and arguments.
 *
 * This function wraps a callable and its arguments into a reactive calculation,
 * which automatically updates when any of its dependencies change.
 *
 * @tparam Fun Callable type.
 * @tparam Args Argument types to be forwarded to the callable.
 * @param fun The callable to invoke reactively.
 * @param args Arguments to forward to the callable.
 * @return A reactive calculation wrapper.
 */
template <typename Fun, typename... Args>
auto create(Fun &&fun, Args &&...args) {
    return calc(std::forward<Fun>(fun), std::forward<Args>(args)...);
}

/**
 * @brief Begins a batch of reactive updates.
 *
 * This function creates a batch context, deferring propagation of reactive updates
 * until the batch is executed. Useful for grouping multiple changes together efficiently.
 *
 * @tparam Fun Callable type to execute within the batch.
 * @param fun A function containing batched operations.
 * @return A Batch object encapsulating the operation.
 */
template <InvocableType Fun>
auto batch(Fun &&fun) {
    return Batch{std::forward<Fun>(fun)};
}

/**
 * @brief Immediately executes a batch of reactive updates.
 *
 * This function creates a batch context and executes it immediately.
 * Any reactive updates are postponed until the end of the batch execution.
 *
 * @tparam Fun Callable type to execute within the batch.
 * @param fun A function containing batched operations to be executed.
 */
template <InvocableType Fun>
void batchExecute(Fun &&fun) {
    Batch batch{std::forward<Fun>(fun)};
    batch.execute();
}

} // namespace reaction
