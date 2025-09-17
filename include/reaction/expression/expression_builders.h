/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/expression/expression_types.h"
#include "reaction/expression/operators.h"
#include "reaction/concept.h"

namespace reaction {

/**
 * @brief Creates a binary expression from two operands and an operator.
 */
template <typename Op, typename L, typename R>
auto make_binary_expr(L &&l, R &&r) {
    return BinaryOpExpr<Op, ExprTraits<std::remove_cvref_t<L>>, ExprTraits<std::remove_cvref_t<R>>>(std::forward<L>(l), std::forward<R>(r));
}

/**
 * @brief Creates a unary expression from one operand and an operator.
 */
template <typename Op, typename T>
auto make_unary_expr(T &&operand) {
    return UnaryOpExpr<Op, ExprTraits<std::remove_cvref_t<T>>>(std::forward<T>(operand));
}

// === Arithmetic Operator Overloads ===

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator+(L &&l, R &&r) {
    return make_binary_expr<AddOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator*(L &&l, R &&r) {
    return make_binary_expr<MulOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator-(L &&l, R &&r) {
    return make_binary_expr<SubOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator/(L &&l, R &&r) {
    return make_binary_expr<DivOp>(std::forward<L>(l), std::forward<R>(r));
}

// === Comparison Operator Overloads ===

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator==(L &&l, R &&r) {
    return make_binary_expr<EqOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator!=(L &&l, R &&r) {
    return make_binary_expr<NeOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator<(L &&l, R &&r) {
    return make_binary_expr<LtOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator>(L &&l, R &&r) {
    return make_binary_expr<GtOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator<=(L &&l, R &&r) {
    return make_binary_expr<LeOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator>=(L &&l, R &&r) {
    return make_binary_expr<GeOp>(std::forward<L>(l), std::forward<R>(r));
}

// === Logical Operator Overloads ===

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator&&(L &&l, R &&r) {
    return make_binary_expr<AndOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator||(L &&l, R &&r) {
    return make_binary_expr<OrOp>(std::forward<L>(l), std::forward<R>(r));
}

// === Unary Operator Overloads ===

template <typename T>
    requires IsReact<std::remove_cvref_t<T>> || IsBinaryOpExpr<std::remove_cvref_t<T>> || IsUnaryOpExpr<std::remove_cvref_t<T>>
auto operator-(T &&operand) {
    return make_unary_expr<NegOp>(std::forward<T>(operand));
}

template <typename T>
    requires IsReact<std::remove_cvref_t<T>> || IsBinaryOpExpr<std::remove_cvref_t<T>> || IsUnaryOpExpr<std::remove_cvref_t<T>>
auto operator!(T &&operand) {
    return make_unary_expr<NotOp>(std::forward<T>(operand));
}

template <typename T>
    requires IsReact<std::remove_cvref_t<T>> || IsBinaryOpExpr<std::remove_cvref_t<T>> || IsUnaryOpExpr<std::remove_cvref_t<T>>
auto operator~(T &&operand) {
    return make_unary_expr<BitNotOp>(std::forward<T>(operand));
}

} // namespace reaction