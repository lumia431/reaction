/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/expression/expression_types.h"
#include "reaction/expression/operators.h"
#include "reaction/core/concept.h"

namespace reaction {

/**
 * @brief Creates a binary expression from two operands and an operator.
 * 
 * Optimized factory function with perfect forwarding and compile-time optimization.
 * 
 * @tparam Op The operator type
 * @tparam L Left operand type
 * @tparam R Right operand type
 * @param l Left operand
 * @param r Right operand
 * @return Binary expression with optimized type deduction
 */
template <typename Op, typename L, typename R>
[[nodiscard]] constexpr auto make_binary_expr(L &&l, R &&r) noexcept(
    std::is_nothrow_constructible_v<BinaryOpExpr<Op, ExprTraits<std::remove_cvref_t<L>>, ExprTraits<std::remove_cvref_t<R>>>, L, R>
) {
    using LeftType = ExprTraits<std::remove_cvref_t<L>>;
    using RightType = ExprTraits<std::remove_cvref_t<R>>;
    return BinaryOpExpr<Op, LeftType, RightType>(std::forward<L>(l), std::forward<R>(r));
}

/**
 * @brief Creates a unary expression from one operand and an operator.
 * 
 * Optimized factory function with perfect forwarding and compile-time optimization.
 * 
 * @tparam Op The operator type
 * @tparam T Operand type
 * @param operand The operand
 * @return Unary expression with optimized type deduction
 */
template <typename Op, typename T>
[[nodiscard]] constexpr auto make_unary_expr(T &&operand) noexcept(
    std::is_nothrow_constructible_v<UnaryOpExpr<Op, ExprTraits<std::remove_cvref_t<T>>>, T>
) {
    using OperandType = ExprTraits<std::remove_cvref_t<T>>;
    return UnaryOpExpr<Op, OperandType>(std::forward<T>(operand));
}

// === Generic Binary Operator Overload Template ===

/**
 * @brief Generic template for binary operator overloads.
 * 
 * Reduces code duplication by providing a unified implementation for all binary operators.
 * 
 * @tparam Op The operator type (AddOp, MulOp, etc.)
 * @tparam L Left operand type
 * @tparam R Right operand type
 */
template <typename Op, typename L, typename R>
    requires HasReactOp<L, R>
constexpr auto make_binary_op(L&& l, R&& r) {
    return make_binary_expr<Op>(std::forward<L>(l), std::forward<R>(r));
}

// === Operator Overload Generation Macros ===

/**
 * @brief Macro to generate binary operator overloads.
 *
 * @param op_symbol The C++ operator symbol
 * @param OpType The corresponding operator functor type
 */
#define REACTION_DEFINE_BINARY_OPERATOR(op_symbol, OpType) \
    template <typename L, typename R> \
        requires HasReactOp<L, R> \
    constexpr auto operator op_symbol(L &&l, R &&r) { \
        return make_binary_op<OpType>(std::forward<L>(l), std::forward<R>(r)); \
    }

/**
 * @brief Macro to generate unary operator overloads.
 *
 * @param op_symbol The C++ operator symbol
 * @param OpType The corresponding operator functor type
 */
#define REACTION_DEFINE_UNARY_OPERATOR(op_symbol, OpType) \
    template <typename T> \
        requires IsReact<std::remove_cvref_t<T>> || IsBinaryOpExpr<std::remove_cvref_t<T>> || IsUnaryOpExpr<std::remove_cvref_t<T>> \
    constexpr auto operator op_symbol(T &&operand) { \
        return make_unary_op<OpType>(std::forward<T>(operand)); \
    }

// === Arithmetic Operators ===

REACTION_DEFINE_BINARY_OPERATOR(+, AddOp);
REACTION_DEFINE_BINARY_OPERATOR(*, MulOp);
REACTION_DEFINE_BINARY_OPERATOR(-, SubOp);
REACTION_DEFINE_BINARY_OPERATOR(/, DivOp);

// === Comparison Operators ===

REACTION_DEFINE_BINARY_OPERATOR(==, EqOp);
REACTION_DEFINE_BINARY_OPERATOR(!=, NeOp);
REACTION_DEFINE_BINARY_OPERATOR(<, LtOp);
REACTION_DEFINE_BINARY_OPERATOR(>, GtOp);
REACTION_DEFINE_BINARY_OPERATOR(<=, LeOp);
REACTION_DEFINE_BINARY_OPERATOR(>=, GeOp);

// === Logical Operators ===

REACTION_DEFINE_BINARY_OPERATOR(&&, AndOp);
REACTION_DEFINE_BINARY_OPERATOR(||, OrOp);

// === Generic Unary Operator Helper ===

/**
 * @brief Generic template for unary operator overloads.
 * 
 * @tparam Op The operator type (NegOp, NotOp, etc.)
 * @tparam T Operand type
 */
template <typename Op, typename T>
    requires IsReact<std::remove_cvref_t<T>> || IsBinaryOpExpr<std::remove_cvref_t<T>> || IsUnaryOpExpr<std::remove_cvref_t<T>>
constexpr auto make_unary_op(T&& operand) {
    return make_unary_expr<Op>(std::forward<T>(operand));
}

// === Unary Operators ===

REACTION_DEFINE_UNARY_OPERATOR(-, NegOp);
REACTION_DEFINE_UNARY_OPERATOR(!, NotOp);
REACTION_DEFINE_UNARY_OPERATOR(~, BitNotOp);

// Cleanup macros to avoid namespace pollution
#undef REACTION_DEFINE_BINARY_OPERATOR
#undef REACTION_DEFINE_UNARY_OPERATOR

} // namespace reaction