/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <type_traits>

namespace reaction {

// === Base Templates for Operator Categories ===

/**
 * @brief Base template for binary operators with automatic forwarding.
 *
 * @tparam Derived The derived operator type (CRTP)
 */
template <typename Derived>
struct BinaryOperatorBase {
    template <typename L, typename R>
    constexpr auto operator()(L &&l, R &&r) const noexcept(noexcept(static_cast<const Derived *>(this)->apply(std::forward<L>(l), std::forward<R>(r)))) {
        return static_cast<const Derived *>(this)->apply(std::forward<L>(l), std::forward<R>(r));
    }
};

/**
 * @brief Base template for unary operators with automatic forwarding.
 *
 * @tparam Derived The derived operator type (CRTP)
 */
template <typename Derived>
struct UnaryOperatorBase {
    template <typename T>
    constexpr auto operator()(T &&operand) const noexcept(noexcept(static_cast<const Derived *>(this)->apply(std::forward<T>(operand)))) {
        return static_cast<const Derived *>(this)->apply(std::forward<T>(operand));
    }
};

// === Operator Generation Macro ===

/**
 * @brief Macro to generate simple binary operators with automatic noexcept deduction.
 *
 * @param OpName The operator class name
 * @param symbol The C++ operator symbol
 * @param description Human-readable description
 */
#define REACTION_DEFINE_BINARY_OP(OpName, symbol, description)                    \
    /** @brief description */                                                     \
    struct OpName : BinaryOperatorBase<OpName> {                                  \
        template <typename L, typename R>                                         \
        constexpr auto apply(L &&l, R &&r) const noexcept(noexcept(l symbol r)) { \
            return l symbol r;                                                    \
        }                                                                         \
    }

// === Arithmetic Operators ===

REACTION_DEFINE_BINARY_OP(AddOp, +, Addition operator functor for reactive expressions);
REACTION_DEFINE_BINARY_OP(MulOp, *, Multiplication operator functor for reactive expressions);
REACTION_DEFINE_BINARY_OP(SubOp, -, Subtraction operator functor for reactive expressions);

/**
 * @brief Division operator functor for reactive expressions.
 *
 * Performs division operation between two operands with floating-point precision.
 * For integer operands, promotes to double to ensure floating-point division.
 * Note: Division by zero behavior depends on the operand types.
 */
struct DivOp : BinaryOperatorBase<DivOp> {
    template <typename L, typename R>
    constexpr auto apply(L &&l, R &&r) const {
        using L_t = std::remove_cvref_t<L>;
        using R_t = std::remove_cvref_t<R>;

        // If both operands are integral types, promote to double for floating-point division
        if constexpr (std::is_integral_v<L_t> && std::is_integral_v<R_t>) {
            return static_cast<double>(l) / static_cast<double>(r);
        } else {
            return l / r;
        }
    }
};

// === Comparison Operators ===

REACTION_DEFINE_BINARY_OP(EqOp, ==, Equality comparison operator functor for reactive expressions);
REACTION_DEFINE_BINARY_OP(NeOp, !=, Inequality comparison operator functor for reactive expressions);
REACTION_DEFINE_BINARY_OP(LtOp, <, Less than comparison operator functor for reactive expressions);
REACTION_DEFINE_BINARY_OP(GtOp, >, Greater than comparison operator functor for reactive expressions);
REACTION_DEFINE_BINARY_OP(LeOp, <=, Less than or equal comparison operator functor for reactive expressions);
REACTION_DEFINE_BINARY_OP(GeOp, >=, Greater than or equal comparison operator functor for reactive expressions);

// === Logical Operators ===

REACTION_DEFINE_BINARY_OP(AndOp, &&, Logical AND operator functor for reactive expressions);
REACTION_DEFINE_BINARY_OP(OrOp, ||, Logical OR operator functor for reactive expressions);

// === Unary Operator Generation Macro ===

/**
 * @brief Macro to generate simple unary operators with automatic noexcept deduction.
 *
 * @param OpName The operator class name
 * @param symbol The C++ operator symbol
 * @param description Human-readable description
 */
#define REACTION_DEFINE_UNARY_OP(OpName, symbol, description)                        \
    /** @brief description */                                                        \
    struct OpName : UnaryOperatorBase<OpName> {                                      \
        template <typename T>                                                        \
        constexpr auto apply(T &&operand) const noexcept(noexcept(symbol operand)) { \
            return symbol operand;                                                   \
        }                                                                            \
    }

// === Unary Operators ===

REACTION_DEFINE_UNARY_OP(NegOp, -, Unary negation operator functor for reactive expressions);
REACTION_DEFINE_UNARY_OP(NotOp, !, Logical NOT operator functor for reactive expressions);
REACTION_DEFINE_UNARY_OP(BitNotOp, ~, Bitwise NOT operator functor for reactive expressions);

// Cleanup macros to avoid pollution
#undef REACTION_DEFINE_BINARY_OP
#undef REACTION_DEFINE_UNARY_OP

} // namespace reaction