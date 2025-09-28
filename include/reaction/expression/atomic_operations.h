/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/core/concept.h"
#include <functional>
#include <type_traits>

namespace reaction {

// Macro definition: Unified generation of compound assignment operators
#define REACTION_DEFINE_COMPOUND_ASSIGN_OP(OpName, symbol)      \
    struct OpName {                                             \
        template <typename T, typename U>                       \
        constexpr void operator()(T &lhs, const U &rhs) const { \
            lhs symbol rhs;                                     \
        }                                                       \
    };

// Generate all compound assignment operators using macro
REACTION_DEFINE_COMPOUND_ASSIGN_OP(AddAssignOp, +=)
REACTION_DEFINE_COMPOUND_ASSIGN_OP(SubtractAssignOp, -=)
REACTION_DEFINE_COMPOUND_ASSIGN_OP(MultiplyAssignOp, *=)
REACTION_DEFINE_COMPOUND_ASSIGN_OP(DivideAssignOp, /=)
REACTION_DEFINE_COMPOUND_ASSIGN_OP(ModuloAssignOp, %=)
REACTION_DEFINE_COMPOUND_ASSIGN_OP(BitwiseAndAssignOp, &=)
REACTION_DEFINE_COMPOUND_ASSIGN_OP(BitwiseOrAssignOp, |=)
REACTION_DEFINE_COMPOUND_ASSIGN_OP(BitwiseXorAssignOp, ^=)
REACTION_DEFINE_COMPOUND_ASSIGN_OP(LeftShiftAssignOp, <<=)
REACTION_DEFINE_COMPOUND_ASSIGN_OP(RightShiftAssignOp, >>=)

// Cleanup macro definition
#undef REACTION_DEFINE_COMPOUND_ASSIGN_OP

/**
 * @brief Generic atomic compound assignment operation helper.
 *
 * This function provides a unified way to perform atomic compound assignment
 * operations on reactive values, handling thread safety, change detection,
 * and notification automatically.
 *
 * @tparam Op The operation type (function object)
 * @tparam Impl The ReactImpl type
 * @tparam U The type of the right-hand side operand
 *
 * @param impl The ReactImpl instance
 * @param rhs The right-hand side value
 *
 * @requires Op must be invocable as Op(Type&, const U&) -> void
 */
template <typename Op, typename Impl, typename U>
constexpr void atomicCompoundAssign(Impl &impl, const U &rhs) {
    impl.atomicOperation([&rhs](auto &val) -> bool {
        if constexpr (ComparableType<std::remove_reference_t<decltype(val)>>) {
            auto oldValue = val;
            Op{}(val, rhs);
            return oldValue != val;
        } else {
            Op{}(val, rhs);
            return true;
        }
    });
}

// Macro definition: Unified generation of atomic compound assignment functions
#define REACTION_DEFINE_ATOMIC_ASSIGN_FUNCTION(name, op_type, operator_symbol)         \
    /**                                                                                \
     * @brief Atomically operator_symbol and assign value (operator_symbol=).          \
     * This prevents race conditions in multi-threaded compound assignment operations. \
     *                                                                                 \
     * @tparam Impl The ReactImpl type                                                 \
     * @tparam U The type of the right-hand side operand                               \
     * @param impl The ReactImpl instance                                              \
     * @param rhs The right-hand side value                                            \
     */                                                                                \
    template <typename Impl, typename U>                                               \
    constexpr void atomic##name##Assign(Impl &impl, const U &rhs) {                    \
        atomicCompoundAssign<op_type, Impl, U>(impl, rhs);                             \
    }

// Define all compound assignment functions
REACTION_DEFINE_ATOMIC_ASSIGN_FUNCTION(Add, AddAssignOp, +)
REACTION_DEFINE_ATOMIC_ASSIGN_FUNCTION(Subtract, SubtractAssignOp, -)
REACTION_DEFINE_ATOMIC_ASSIGN_FUNCTION(Multiply, MultiplyAssignOp, *)
REACTION_DEFINE_ATOMIC_ASSIGN_FUNCTION(Divide, DivideAssignOp, /)
REACTION_DEFINE_ATOMIC_ASSIGN_FUNCTION(Modulo, ModuloAssignOp, %)
REACTION_DEFINE_ATOMIC_ASSIGN_FUNCTION(BitwiseAnd, BitwiseAndAssignOp, &)
REACTION_DEFINE_ATOMIC_ASSIGN_FUNCTION(BitwiseOr, BitwiseOrAssignOp, |)
REACTION_DEFINE_ATOMIC_ASSIGN_FUNCTION(BitwiseXor, BitwiseXorAssignOp, ^)
REACTION_DEFINE_ATOMIC_ASSIGN_FUNCTION(LeftShift, LeftShiftAssignOp, <<)
REACTION_DEFINE_ATOMIC_ASSIGN_FUNCTION(RightShift, RightShiftAssignOp, >>)

// Cleanup macro definition
#undef REACTION_DEFINE_ATOMIC_ASSIGN_FUNCTION

/**
 * @brief Atomically increment the value.
 * This prevents race conditions in multi-threaded increment operations.
 *
 * @tparam Impl The ReactImpl type
 * @param impl The ReactImpl instance
 */
template <typename Impl>
constexpr void atomicIncrement(Impl &impl) {
    impl.atomicOperation([](auto &val) -> bool {
        ++val;
        return true; // Increment always represents a change
    },
        true);
}

/**
 * @brief Atomically post-increment the value.
 * This prevents race conditions in multi-threaded increment operations.
 *
 * @tparam Impl The ReactImpl type
 * @param impl The ReactImpl instance
 * @return The old value before increment
 */
template <typename Impl>
constexpr auto atomicPostIncrement(Impl &impl) {
    using ValueType = std::decay_t<decltype(impl.get())>;
    ValueType oldValue{};

    impl.atomicOperation([&oldValue](auto &val) -> bool {
        oldValue = val;
        ++val;
        return true; // Increment always represents a change
    },
        true);
    return oldValue;
}

/**
 * @brief Atomically decrement the value.
 * This prevents race conditions in multi-threaded decrement operations.
 *
 * @tparam Impl The ReactImpl type
 * @param impl The ReactImpl instance
 */
template <typename Impl>
constexpr void atomicDecrement(Impl &impl) {
    impl.atomicOperation([](auto &val) -> bool {
        --val;
        return true; // Decrement always represents a change
    },
        true);
}

/**
 * @brief Atomically post-decrement the value.
 * This prevents race conditions in multi-threaded decrement operations.
 *
 * @tparam Impl The ReactImpl type
 * @param impl The ReactImpl instance
 * @return The old value before decrement
 */
template <typename Impl>
constexpr auto atomicPostDecrement(Impl &impl) {
    using ValueType = std::decay_t<decltype(impl.get())>;
    ValueType oldValue{};

    impl.atomicOperation([&oldValue](auto &val) -> bool {
        oldValue = val;
        --val;
        return true; // Decrement always represents a change
    },
        true);
    return oldValue;
}

} // namespace reaction
