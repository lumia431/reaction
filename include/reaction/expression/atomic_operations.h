/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <functional>
#include <type_traits>

namespace reaction {

// Helper function objects for all assignment operations
struct AddAssignOp {
    template <typename T, typename U>
    void operator()(T &lhs, const U &rhs) const {
        lhs += rhs;
    }
};

struct SubtractAssignOp {
    template <typename T, typename U>
    void operator()(T &lhs, const U &rhs) const {
        lhs -= rhs;
    }
};

struct MultiplyAssignOp {
    template <typename T, typename U>
    void operator()(T &lhs, const U &rhs) const {
        lhs *= rhs;
    }
};

struct DivideAssignOp {
    template <typename T, typename U>
    void operator()(T &lhs, const U &rhs) const {
        lhs /= rhs;
    }
};

struct ModuloAssignOp {
    template <typename T, typename U>
    void operator()(T &lhs, const U &rhs) const {
        lhs %= rhs;
    }
};

struct BitwiseAndAssignOp {
    template <typename T, typename U>
    void operator()(T &lhs, const U &rhs) const {
        lhs &= rhs;
    }
};

struct BitwiseOrAssignOp {
    template <typename T, typename U>
    void operator()(T &lhs, const U &rhs) const {
        lhs |= rhs;
    }
};

struct BitwiseXorAssignOp {
    template <typename T, typename U>
    void operator()(T &lhs, const U &rhs) const {
        lhs ^= rhs;
    }
};

struct LeftShiftAssignOp {
    template <typename T, typename U>
    void operator()(T &lhs, const U &rhs) const {
        lhs <<= rhs;
    }
};

struct RightShiftAssignOp {
    template <typename T, typename U>
    void operator()(T &lhs, const U &rhs) const {
        lhs >>= rhs;
    }
};

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
void atomicCompoundAssign(Impl &impl, const U &rhs) {
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

/**
 * @brief Atomically add and assign value (+=).
 * This prevents race conditions in multi-threaded compound assignment operations.
 *
 * @tparam Impl The ReactImpl type
 * @tparam U The type of the right-hand side operand
 * @param impl The ReactImpl instance
 * @param rhs The right-hand side value
 */
template <typename Impl, typename U>
void atomicAddAssign(Impl &impl, const U &rhs) {
    atomicCompoundAssign<AddAssignOp, Impl, U>(impl, rhs);
}

/**
 * @brief Atomically subtract and assign value (-=).
 * This prevents race conditions in multi-threaded compound assignment operations.
 *
 * @tparam Impl The ReactImpl type
 * @tparam U The type of the right-hand side operand
 * @param impl The ReactImpl instance
 * @param rhs The right-hand side value
 */
template <typename Impl, typename U>
void atomicSubtractAssign(Impl &impl, const U &rhs) {
    atomicCompoundAssign<SubtractAssignOp, Impl, U>(impl, rhs);
}

/**
 * @brief Atomically multiply and assign value (*=).
 * This prevents race conditions in multi-threaded compound assignment operations.
 *
 * @tparam Impl The ReactImpl type
 * @tparam U The type of the right-hand side operand
 * @param impl The ReactImpl instance
 * @param rhs The right-hand side value
 */
template <typename Impl, typename U>
void atomicMultiplyAssign(Impl &impl, const U &rhs) {
    atomicCompoundAssign<MultiplyAssignOp, Impl, U>(impl, rhs);
}

/**
 * @brief Atomically divide and assign value (/=).
 * This prevents race conditions in multi-threaded compound assignment operations.
 *
 * @tparam Impl The ReactImpl type
 * @tparam U The type of the right-hand side operand
 * @param impl The ReactImpl instance
 * @param rhs The right-hand side value
 */
template <typename Impl, typename U>
void atomicDivideAssign(Impl &impl, const U &rhs) {
    atomicCompoundAssign<DivideAssignOp, Impl, U>(impl, rhs);
}

/**
 * @brief Atomically modulo and assign value (%=).
 * This prevents race conditions in multi-threaded compound assignment operations.
 *
 * @tparam Impl The ReactImpl type
 * @tparam U The type of the right-hand side operand
 * @param impl The ReactImpl instance
 * @param rhs The right-hand side value
 */
template <typename Impl, typename U>
void atomicModuloAssign(Impl &impl, const U &rhs) {
    atomicCompoundAssign<ModuloAssignOp, Impl, U>(impl, rhs);
}

/**
 * @brief Atomically bitwise AND and assign value (&=).
 * This prevents race conditions in multi-threaded compound assignment operations.
 *
 * @tparam Impl The ReactImpl type
 * @tparam U The type of the right-hand side operand
 * @param impl The ReactImpl instance
 * @param rhs The right-hand side value
 */
template <typename Impl, typename U>
void atomicBitwiseAndAssign(Impl &impl, const U &rhs) {
    atomicCompoundAssign<BitwiseAndAssignOp, Impl, U>(impl, rhs);
}

/**
 * @brief Atomically bitwise OR and assign value (|=).
 * This prevents race conditions in multi-threaded compound assignment operations.
 *
 * @tparam Impl The ReactImpl type
 * @tparam U The type of the right-hand side operand
 * @param impl The ReactImpl instance
 * @param rhs The right-hand side value
 */
template <typename Impl, typename U>
void atomicBitwiseOrAssign(Impl &impl, const U &rhs) {
    atomicCompoundAssign<BitwiseOrAssignOp, Impl, U>(impl, rhs);
}

/**
 * @brief Atomically bitwise XOR and assign value (^=).
 * This prevents race conditions in multi-threaded compound assignment operations.
 *
 * @tparam Impl The ReactImpl type
 * @tparam U The type of the right-hand side operand
 * @param impl The ReactImpl instance
 * @param rhs The right-hand side value
 */
template <typename Impl, typename U>
void atomicBitwiseXorAssign(Impl &impl, const U &rhs) {
    atomicCompoundAssign<BitwiseXorAssignOp, Impl, U>(impl, rhs);
}

/**
 * @brief Atomically left shift and assign value (<<=).
 * This prevents race conditions in multi-threaded compound assignment operations.
 *
 * @tparam Impl The ReactImpl type
 * @tparam U The type of the right-hand side operand
 * @param impl The ReactImpl instance
 * @param rhs The right-hand side value
 */
template <typename Impl, typename U>
void atomicLeftShiftAssign(Impl &impl, const U &rhs) {
    atomicCompoundAssign<LeftShiftAssignOp, Impl, U>(impl, rhs);
}

/**
 * @brief Atomically right shift and assign value (>>=).
 * This prevents race conditions in multi-threaded compound assignment operations.
 *
 * @tparam Impl The ReactImpl type
 * @tparam U The type of the right-hand side operand
 * @param impl The ReactImpl instance
 * @param rhs The right-hand side value
 */
template <typename Impl, typename U>
void atomicRightShiftAssign(Impl &impl, const U &rhs) {
    atomicCompoundAssign<RightShiftAssignOp, Impl, U>(impl, rhs);
}

/**
 * @brief Atomically increment the value.
 * This prevents race conditions in multi-threaded increment operations.
 *
 * @tparam Impl The ReactImpl type
 * @param impl The ReactImpl instance
 */
template <typename Impl>
void atomicIncrement(Impl &impl) {
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
auto atomicPostIncrement(Impl &impl) {
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
void atomicDecrement(Impl &impl) {
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
auto atomicPostDecrement(Impl &impl) {
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
