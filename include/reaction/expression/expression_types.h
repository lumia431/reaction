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
 * @brief Enhanced base template for expression evaluation with optimized performance.
 *
 * Uses CRTP pattern for zero-cost abstraction and compile-time polymorphism.
 *
 * @tparam Derived The derived expression type (CRTP)
 * @tparam ValueType The type of value this expression evaluates to
 */
template <typename Derived, typename ValueType> class ExpressionBase {
public:
    using value_type = ValueType;

    /// @brief Evaluates the expression by calling the derived implementation.
    [[nodiscard]] constexpr auto operator()() const
        noexcept(noexcept(static_cast<const Derived*>(this)->evaluate())) {
        return static_cast<const Derived*>(this)->evaluate();
    }

    /// @brief Implicit conversion to value type (evaluates expression).
    [[nodiscard]] constexpr operator value_type() const
        noexcept(noexcept(static_cast<const Derived*>(this)->evaluate())) {
        return static_cast<const Derived*>(this)->evaluate();
    }

    /// @brief Explicit evaluation method for clarity.
    [[nodiscard]] constexpr auto evaluate() const
        noexcept(noexcept(static_cast<const Derived*>(this)->evaluate())) {
        return static_cast<const Derived*>(this)->evaluate();
    }
};

/**
 * @brief Optimized wrapper for literal values into callable expressions.
 *
 * Provides zero-overhead abstraction for constant values in expression trees.
 *
 * @tparam T Value type.
 */
template <typename T>
struct ValueWrapper : ExpressionBase<ValueWrapper<T>, std::remove_cvref_t<T>> {
    using value_type = std::remove_cvref_t<T>;

    static_assert(!std::is_reference_v<T>, "ValueWrapper should not store references");

    T m_value;

    template <typename Type>
    requires Convertable<Type, T>
    constexpr ValueWrapper(Type&& t) noexcept(std::is_nothrow_constructible_v<T, Type>)
        : m_value(std::forward<Type>(t)) {}

    [[nodiscard]] constexpr const T& evaluate() const noexcept { return m_value; }

    [[nodiscard]] constexpr const T& getValue() const noexcept { return m_value; }
};

/**
 * @brief Enhanced unary operator wrapper for reactive expression trees.
 *
 * Optimized for performance with better memory layout and noexcept specifications.
 *
 * @tparam Op The unary operation (e.g., NegOp).
 * @tparam T  Operand expression type.
 */
template <typename Op, typename T>
class UnaryOpExpr : public ExpressionBase<UnaryOpExpr<Op, T>, typename T::value_type> {
public:
    using value_type    = typename T::value_type;
    using operand_type  = T;
    using operator_type = Op;

    template <typename Operand>
    requires Convertable<Operand, T>
    constexpr UnaryOpExpr(Operand&& operand,
                          Op op = Op{}) noexcept(std::is_nothrow_constructible_v<T, Operand> &&
                                                 std::is_nothrow_constructible_v<Op>)
        : m_operand(std::forward<Operand>(operand)), m_op(op) {}

    /// @brief Evaluates the unary expression with optimal performance.
    [[nodiscard]] constexpr auto evaluate() const noexcept(noexcept(m_op(m_operand()))) {
        return m_op(m_operand());
    }

    /// @brief Access to the operand for introspection.
    [[nodiscard]] constexpr const T& getOperand() const noexcept { return m_operand; }

private:
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
class BinaryOpExpr
    : public ExpressionBase<
          BinaryOpExpr<Op, L, R>,
          typename std::conditional_t<
              std::is_same_v<Op, DivOp> && std::is_integral_v<typename L::value_type> &&
                  std::is_integral_v<typename R::value_type>,
              double,
              std::remove_cvref_t<
                  std::common_type_t<typename L::value_type, typename R::value_type>>>> {
public:
    // Special type deduction for division: promote integral types to double for floating-point
    // division
    using value_type = typename std::conditional_t<
        std::is_same_v<Op, DivOp> && std::is_integral_v<typename L::value_type> &&
            std::is_integral_v<typename R::value_type>,
        double,
        std::remove_cvref_t<std::common_type_t<typename L::value_type, typename R::value_type>>>;

    template <typename Left, typename Right>
    constexpr BinaryOpExpr(Left&& l, Right&& r, Op o = Op{})
        : m_left(std::forward<Left>(l)), m_right(std::forward<Right>(r)), m_op(o) {}

    /// @brief Evaluates the binary expression.
    constexpr auto evaluate() const noexcept(noexcept(m_op(m_left(), m_right()))) {
        return m_op(m_left(), m_right());
    }

private:
    L m_left;
    R m_right;
    [[no_unique_address]] Op m_op;
};

} // namespace reaction