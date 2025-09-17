/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <type_traits>

namespace reaction {

// === Arithmetic Operators ===

/**
 * @brief Addition operator functor for reactive expressions.
 *
 * Performs addition operation between two operands of potentially different types.
 * The result type is determined by C++ type promotion rules.
 */
struct AddOp {
    auto operator()(auto &&l, auto &&r) const {
        return l + r;
    }
};

/**
 * @brief Multiplication operator functor for reactive expressions.
 *
 * Performs multiplication operation between two operands.
 */
struct MulOp {
    auto operator()(auto &&l, auto &&r) const {
        return l * r;
    }
};

/**
 * @brief Subtraction operator functor for reactive expressions.
 *
 * Performs subtraction operation between two operands.
 */
struct SubOp {
    auto operator()(auto &&l, auto &&r) const {
        return l - r;
    }
};

/**
 * @brief Division operator functor for reactive expressions.
 *
 * Performs division operation between two operands with floating-point precision.
 * For integer operands, promotes to double to ensure floating-point division.
 * Note: Division by zero behavior depends on the operand types.
 */
struct DivOp {
    auto operator()(auto &&l, auto &&r) const {
        using L = std::remove_cvref_t<decltype(l)>;
        using R = std::remove_cvref_t<decltype(r)>;

        // If both operands are integral types, promote to double for floating-point division
        if constexpr (std::is_integral_v<L> && std::is_integral_v<R>) {
            return static_cast<double>(l) / static_cast<double>(r);
        } else {
            return l / r;
        }
    }
};

// === Comparison Operators ===

/**
 * @brief Equality comparison operator functor for reactive expressions.
 *
 * Performs equality comparison between two operands.
 */
struct EqOp {
    auto operator()(auto &&l, auto &&r) const {
        return l == r;
    }
};

/**
 * @brief Inequality comparison operator functor for reactive expressions.
 *
 * Performs inequality comparison between two operands.
 */
struct NeOp {
    auto operator()(auto &&l, auto &&r) const {
        return l != r;
    }
};

/**
 * @brief Less than comparison operator functor for reactive expressions.
 *
 * Performs less than comparison between two operands.
 */
struct LtOp {
    auto operator()(auto &&l, auto &&r) const {
        return l < r;
    }
};

/**
 * @brief Greater than comparison operator functor for reactive expressions.
 *
 * Performs greater than comparison between two operands.
 */
struct GtOp {
    auto operator()(auto &&l, auto &&r) const {
        return l > r;
    }
};

/**
 * @brief Less than or equal comparison operator functor for reactive expressions.
 *
 * Performs less than or equal comparison between two operands.
 */
struct LeOp {
    auto operator()(auto &&l, auto &&r) const {
        return l <= r;
    }
};

/**
 * @brief Greater than or equal comparison operator functor for reactive expressions.
 *
 * Performs greater than or equal comparison between two operands.
 */
struct GeOp {
    auto operator()(auto &&l, auto &&r) const {
        return l >= r;
    }
};

// === Logical Operators ===

/**
 * @brief Logical AND operator functor for reactive expressions.
 *
 * Performs logical AND operation between two operands.
 */
struct AndOp {
    auto operator()(auto &&l, auto &&r) const {
        return l && r;
    }
};

/**
 * @brief Logical OR operator functor for reactive expressions.
 *
 * Performs logical OR operation between two operands.
 */
struct OrOp {
    auto operator()(auto &&l, auto &&r) const {
        return l || r;
    }
};

// === Unary Operators ===

/**
 * @brief Unary negation operator functor for reactive expressions.
 *
 * Performs unary negation operation on operand.
 */
struct NegOp {
    auto operator()(auto &&operand) const {
        return -operand;
    }
};

/**
 * @brief Logical NOT operator functor for reactive expressions.
 *
 * Performs logical NOT operation on operand.
 */
struct NotOp {
    auto operator()(auto &&operand) const {
        return !operand;
    }
};

/**
 * @brief Bitwise NOT operator functor for reactive expressions.
 *
 * Performs bitwise NOT operation on operand.
 */
struct BitNotOp {
    auto operator()(auto &&operand) const {
        return ~operand;
    }
};

} // namespace reaction