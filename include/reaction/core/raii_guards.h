/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <type_traits>

#include "reaction/concurrency/global_state.h"

namespace reaction {

// === RAII Guard Templates ===

/**
 * @brief Unified RAII guard template for managing global state variables.
 *
 * This template provides automatic cleanup with both runtime and compile-time optimization.
 * Combines the functionality of both GlobalStateGuard and StaticGlobalGuard for better performance.
 *
 * @tparam T Type of the global variable
 */
template <typename T> class UnifiedStateGuard {
public:
    /**
     * @brief Constructor for runtime configuration.
     *
     * @param var Reference to the global variable to manage
     * @param value Value to assign to the global variable
     * @param default_value Default value to restore when destructor is called
     */
    template <typename U>
    requires(!std::is_same_v<std::remove_cvref_t<U>, UnifiedStateGuard>)
    explicit UnifiedStateGuard(T& var, U&& value, const T& default_value) noexcept(
        std::is_nothrow_assignable_v<T&, U> && std::is_nothrow_copy_constructible_v<T>)
        : m_var(var), m_default_value(default_value) {
        m_var = std::forward<U>(value);
    }

    /**
     * @brief Destructor that restores the global variable to its default value.
     */
    ~UnifiedStateGuard() noexcept(std::is_nothrow_assignable_v<T&, const T&>) {
        m_var = m_default_value;
    }

    // Disable copying and moving for safety
    UnifiedStateGuard(const UnifiedStateGuard&)            = delete;
    UnifiedStateGuard& operator=(const UnifiedStateGuard&) = delete;
    UnifiedStateGuard(UnifiedStateGuard&&)                 = delete;
    UnifiedStateGuard& operator=(UnifiedStateGuard&&)      = delete;

private:
    T& m_var;
    T m_default_value;
};

/**
 * @brief Optimized compile-time RAII guard for managing global state variables.
 *
 * This template provides automatic cleanup with compile-time default values,
 * offering better performance for known default values.
 *
 * @tparam VarType Type of the variable to manage
 * @tparam DefaultVal Default value to restore when destructor is called
 */
template <typename VarType, auto DefaultVal> struct OptimizedGlobalGuard {
    /**
     * @brief Constructor that sets the global variable to the provided value.
     *
     * @tparam T Type of the value to set (deduced)
     * @param var_ref Reference to the global variable to manage
     * @param value Value to assign to the global variable
     */
    template <typename T>
    requires(!std::is_same_v<std::remove_cvref_t<T>, OptimizedGlobalGuard>)
    explicit OptimizedGlobalGuard(VarType& var_ref,
                                  T&& value) noexcept(std::is_nothrow_assignable_v<VarType, T>)
        : m_var(var_ref) {
        m_var = std::forward<T>(value);
    }

    /**
     * @brief Destructor that restores the global variable to its default value.
     */
    ~OptimizedGlobalGuard() noexcept(std::is_nothrow_assignable_v<VarType, decltype(DefaultVal)>) {
        m_var = DefaultVal;
    }

    // Disable copying and moving
    OptimizedGlobalGuard(const OptimizedGlobalGuard&)            = delete;
    OptimizedGlobalGuard& operator=(const OptimizedGlobalGuard&) = delete;
    OptimizedGlobalGuard(OptimizedGlobalGuard&&)                 = delete;
    OptimizedGlobalGuard& operator=(OptimizedGlobalGuard&&)      = delete;

private:
    VarType& m_var;
};

// === Specific RAII Guard Factory Functions ===

/**
 * @brief Helper function to create RAII guard for dependency registration function.
 *
 * @tparam T Type of the value to set
 * @param value Value to assign to the global registration function
 * @return RAII guard that manages the registration function state
 */
template <typename T> [[nodiscard]] auto makeRegFunGuard(T&& value) noexcept {
    return UnifiedStateGuard<std::function<void(const NodePtr&)>>(
        g_reg_fun, std::forward<T>(value), nullptr);
}

/**
 * @brief Helper function to create RAII guard for batch function tracking.
 *
 * @tparam T Type of the value to set
 * @param value Value to assign to the global batch function
 * @return RAII guard that manages the batch function state
 */
template <typename T> [[nodiscard]] auto makeBatchFunGuard(T&& value) noexcept {
    return UnifiedStateGuard<std::function<void(const NodePtr&)>>(
        g_batch_fun, std::forward<T>(value), nullptr);
}

/**
 * @brief Helper function to create RAII guard for batch execution flag.
 *
 * @param value Value to assign to the global batch execution flag
 * @return RAII guard that manages the batch execution state
 */
[[nodiscard]] inline auto makeBatchExeGuard(bool value) noexcept {
    return UnifiedStateGuard<bool>(g_batch_execute, value, false);
}

/**
 * @brief Optimized helper using compile-time guard for batch execution.
 *
 * @param value Value to assign to the global batch execution flag
 * @return Static RAII guard with compile-time default value
 */
[[nodiscard]] inline auto makeStaticBatchExeGuard(bool value) noexcept {
    return OptimizedGlobalGuard<bool, false>(g_batch_execute, value);
}

// === Convenience Type Aliases ===

/**
 * @brief Type alias for dependency registration guard.
 */
using DepRegGuard = UnifiedStateGuard<std::function<void(const NodePtr&)>>;

/**
 * @brief Type alias for batch function guard.
 */
using BatchFunGuard = UnifiedStateGuard<std::function<void(const NodePtr&)>>;

/**
 * @brief Type alias for batch execution guard.
 */
using BatchExeGuard = UnifiedStateGuard<bool>;

} // namespace reaction
