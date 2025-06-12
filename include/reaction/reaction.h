/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/react.h"

namespace reaction {

/**
 * @brief Alias template for a reactive variable (Var) based on VarExpr expression type.
 *
 * @tparam SrcType The underlying data type held by this reactive variable.
 * @tparam IS Invalidation strategy, default is KeepStra.
 * @tparam TM Trigger mode, default is ChangeTrig.
 */
template <typename SrcType, IsInvaStra IS = KeepStra, IsTrigMode TM = ChangeTrig>
using Var = React<VarExpr, SrcType, IS, TM>;

/**
 * @brief Alias template for a reactive expresstion (Expr) based on BinaryOpExpr expression type.
 *
 * @tparam SrcType The underlying data type produced by this reactive calculation.
 * @tparam IS Invalidation strategy, default is KeepStra.
 * @tparam TM Trigger mode, default is ChangeTrig.
 */
template <typename SrcType, IsInvaStra IS = KeepStra, IsTrigMode TM = ChangeTrig>
using Calc = React<CalcExpr, SrcType, IS, TM>;

/**
 * @brief Alias template for a reactive action (Action) based on CalcExpr expression type.
 *
 * @tparam IS Invalidation strategy, default is KeepStra.
 * @tparam TM Trigger mode, default is ChangeTrig.
 */
template <IsInvaStra IS = KeepStra, IsTrigMode TM = ChangeTrig>
using Action = React<CalcExpr, Void, IS, TM>;

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
     * @tparam TM Trigger mode, default is ChangeTrig.
     * @tparam IS Invalidation strategy, default is KeepStra.
     * @tparam T The type of the initial value (deduced).
     * @param t The initial value to store in the field.
     * @return React<VarExpr, std::decay_t<T>, IS, TM> A reactive wrapper around the stored value.
     */
    template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = KeepStra, NonReact T>
    auto field(T &&t) {
        auto ptr = std::make_shared<ReactImpl<VarExpr, std::decay_t<T>, IS, TM>>(std::forward<T>(t));
        ObserverGraph::getInstance().addNode(ptr->shared_from_this());
        FieldGraph::getInstance().addObj(m_id, ptr->shared_from_this());
        return React(ptr);
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
 * @tparam TM Trigger mode, default is ChangeTrig.
 * @tparam IS Invalidation strategy, default is KeepStra.
 * @tparam SrcType The type of the source value.
 * @param t The value to wrap as a const reactive variable.
 * @return React<VarExpr, const std::decay_t<SrcType>, IS, TM> Reactive constant wrapper.
 */
template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = KeepStra, NonReact SrcType>
auto constVar(SrcType &&t) {
    auto ptr = std::make_shared<ReactImpl<VarExpr, const std::decay_t<SrcType>, IS, TM>>(std::forward<SrcType>(t));
    ObserverGraph::getInstance().addNode(ptr);
    return React(ptr);
}

/**
 * @brief Create a reactive variable wrapping the given value.
 *
 * If the source type supports the Field concept, binds the field in the FieldGraph.
 * Registers the reactive node to the ObserverGraph.
 *
 * @tparam TM Trigger mode, default is ChangeTrig.
 * @tparam IS Invalidation strategy, default is KeepStra.
 * @tparam SrcType The type of the source value.
 * @param t The value to wrap reactively.
 * @return React<VarExpr, std::decay_t<SrcType>, IS, TM> Reactive variable wrapper.
 */
template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = KeepStra, NonReact SrcType>
auto var(SrcType &&t) {
    auto ptr = std::make_shared<ReactImpl<VarExpr, std::decay_t<SrcType>, IS, TM>>(std::forward<SrcType>(t));
    ObserverGraph::getInstance().addNode(ptr);
    if constexpr (HasField<SrcType>) {
        FieldGraph::getInstance().bindField(t.getId(), ptr->shared_from_this());
    }
    return React(ptr);
}

/**
 * @brief Create a reactive expression wrapping the given operator expression.
 *
 * Registers the node to ObserverGraph, then sets (evaluates) the expression immediately.
 *
 * @tparam TM Trigger mode, default is ChangeTrig.
 * @tparam IS Invalidation strategy, default is KeepStra.
 * @tparam OpExpr The operator expression type.
 * @param opExpr The operator expression to wrap.
 * @return React<CalcExpr, std::decay_t<OpExpr>, IS, TM> Reactive calculation wrapper.
 */
template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = KeepStra, IsBinaryOpExpr OpExpr>
auto expr(OpExpr &&opExpr) {
    auto ptr = std::make_shared<ReactImpl<CalcExpr, std::decay_t<OpExpr>, IS, TM>>(std::forward<OpExpr>(opExpr));
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set();
    return React{ptr};
}

/**
 * @brief Create a reactive calculation based on a callable and its arguments.
 *
 * Registers the node to ObserverGraph and immediately sets (evaluates) the calculation.
 *
 * @tparam TM Trigger mode, default is ChangeTrig.
 * @tparam IS Invalidation strategy, default is KeepStra.
 * @tparam Fun Callable type.
 * @tparam Args Argument types for the callable.
 * @param fun The callable to invoke reactively.
 * @param args Arguments to forward to the callable.
 * @return React<CalcExpr, ReturnType<Fun, Args...>, IS, TM> Reactive calculation wrapper.
 */
template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = KeepStra, typename Fun, typename... Args>
auto calc(Fun &&fun, Args &&...args) {
    auto ptr = std::make_shared<ReactImpl<CalcExpr, ReturnType<Fun, Args...>, IS, TM>>();
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set(std::forward<Fun>(fun), std::forward<Args>(args)...);
    return React(ptr);
}

/**
 * @brief Alias for calc(), used to create a reactive action.
 *
 * @tparam TM Trigger mode, default is ChangeTrig.
 * @tparam IS Invalidation strategy, default is KeepStra.
 * @tparam Fun Callable type.
 * @tparam Args Argument types.
 * @param fun The callable to invoke reactively.
 * @param args Arguments to forward to the callable.
 * @return The reactive action wrapper.
 */
template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = KeepStra, typename Fun, typename... Args>
auto action(Fun &&fun, Args &&...args) {
    return calc<TM, IS>(std::forward<Fun>(fun), std::forward<Args>(args)...);
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
template <typename T>
auto create(T &&t) {
    if constexpr (IsBinaryOpExpr<std::decay_t<T>>) {
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
 * @tparam Fun Callable type.
 * @tparam Args Argument types.
 * @param fun The callable to invoke reactively.
 * @param args Arguments to forward to the callable.
 * @return Reactive calculation wrapper.
 */
template <typename Fun, typename... Args>
auto create(Fun &&fun, Args &&...args) {
    return calc(std::forward<Fun>(fun), std::forward<Args>(args)...);
}

} // namespace reaction
