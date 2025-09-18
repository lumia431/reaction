/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <type_traits>
#include "reaction/expression/operators.h"

namespace reaction {

// === Forward Declarations ===
struct DivOp;

/**
 * @brief Wraps a literal value into a callable expression.
 *
 * @tparam T Value type.
 */
template <typename T>
struct ValueWrapper {
    using value_type = std::remove_cvref_t<T>;
    T m_value;

    template <typename Type>
    ValueWrapper(Type &&t) : m_value(std::forward<Type>(t)) {}

    const T &operator()() const {
        return m_value;
    }
};

/**
 * @brief Unary operator wrapper for reactive expression trees.
 *
 * Encapsulates a single operand with an operator to form an evaluatable expression.
 *
 * @tparam Op The unary operation (e.g., NegOp).
 * @tparam T  Operand expression type.
 */
template <typename Op, typename T>
class UnaryOpExpr {
public:
    using value_type = typename T::value_type;

    template <typename Operand>
    UnaryOpExpr(Operand &&operand, Op o = Op{})
        : m_operand(std::forward<Operand>(operand)), m_op(o) {}

    /// @brief Evaluates the expression.
    auto operator()() const noexcept {
        return calculate();
    }

    /// @brief Implicit conversion to value type (evaluates expression).
    operator value_type() {
        return calculate();
    }

private:
    auto calculate() const noexcept {
        return m_op(m_operand());
    }

    T m_operand;
    [[no_unique_address]] Op m_op;
};

/**
 * @brief Binary operator wrapper for reactive expression trees.
 *
 * Encapsulates left and right operands with an operator to form an evaluatable expression.
 *
 * @tparam Op The binary operation (e.g., AddOp).
 * @tparam L  Left-hand side expression type.
 * @tparam R  Right-hand side expression type.
 */
template <typename Op, typename L, typename R>
class BinaryOpExpr {
public:
    // Special type deduction for division: promote integral types to double for floating-point division
    using value_type = typename std::conditional_t<
        std::is_same_v<Op, DivOp> &&
        std::is_integral_v<typename L::value_type> &&
        std::is_integral_v<typename R::value_type>,
        double,
        std::remove_cvref_t<std::common_type_t<typename L::value_type, typename R::value_type>>
    >;

    template <typename Left, typename Right>
    BinaryOpExpr(Left &&l, Right &&r, Op o = Op{})
        : m_left(std::forward<Left>(l)), m_right(std::forward<Right>(r)), m_op(o) {}

    /// @brief Evaluates the expression.
    auto operator()() const noexcept {
        return calculate();
    }

    /// @brief Implicit conversion to value type (evaluates expression).
    operator value_type() {
        return calculate();
    }

private:
    auto calculate() const noexcept {
        return m_op(m_left(), m_right());
    }

    L m_left;
    R m_right;
    [[no_unique_address]] Op m_op;
};

} // namespace reaction