/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/core/types.h"
#include <functional>

namespace reaction {

// === Global State Management for Reactive System ===

/**
 * @brief RAII guard for managing global state variables.
 *
 * Provides automatic cleanup of global variables when the guard goes out of scope.
 * Used to temporarily set global state and ensure it's restored to a default value.
 *
 * @tparam val Reference to the global variable to manage
 * @tparam guard Default value to restore when destructor is called
 */
template <auto &val, auto guard>
struct GlobalGuard {
    /**
     * @brief Constructor that sets the global variable to the provided value.
     * @tparam T Type of the value to set (deduced)
     * @param t Value to assign to the global variable
     */
    template <typename T>
        requires(!std::is_same_v<std::remove_cvref_t<T>, GlobalGuard>)
    explicit GlobalGuard(T &&t) {
        val = std::forward<T>(t);
    }

    /**
     * @brief Destructor that restores the global variable to its guard value.
     */
    ~GlobalGuard() {
        val = guard;
    }

    // Non-copyable, non-movable
    GlobalGuard(const GlobalGuard &) = delete;
    GlobalGuard(GlobalGuard &&) = delete;
    GlobalGuard &operator=(const GlobalGuard &) = delete;
    GlobalGuard &operator=(GlobalGuard &&) = delete;
};

// === Thread-Local Global State Variables ===

/**
 * @brief Function pointer for dependency registration.
 *
 * Called whenever a reactive value is accessed to enable automatic dependency
 * tracking during expression evaluation. Thread-local for isolation between threads.
 */
inline std::function<void(const NodePtr &)> g_reg_fun = nullptr;

/**
 * @brief Function pointer for batch operation tracking.
 *
 * Called when reactive values are modified during batch operations to enable
 * deferred notification handling. Thread-local for per-thread batch operations.
 */
inline std::function<void(const NodePtr &)> g_batch_fun = nullptr;

/**
 * @brief Flag indicating whether a batch operation is currently executing.
 *
 * When true, reactive notifications are deferred until batch completion.
 * Thread-local to allow independent batch operations per thread.
 */
inline bool g_batch_execute = false;

// === Type Aliases for RAII Guards ===

/// RAII guard for dependency registration function
using RegFunGuard = GlobalGuard<g_reg_fun, nullptr>;

/// RAII guard for batch function tracking
using BatchFunGuard = GlobalGuard<g_batch_fun, nullptr>;

/// RAII guard for batch execution flag
using BatchExeGuard = GlobalGuard<g_batch_execute, false>;

// === Global State Query Functions ===

/**
 * @brief Check if dependency registration is active.
 * @return true if dependency registration function is set
 */
[[nodiscard]] inline bool isDependencyTrackingActive() noexcept {
    return g_reg_fun != nullptr;
}

/**
 * @brief Check if batch operation is active.
 * @return true if batch operation is currently executing
 */
[[nodiscard]] inline bool isBatchActive() noexcept {
    return g_batch_execute;
}

/**
 * @brief Check if batch function tracking is active.
 * @return true if batch function is set
 */
[[nodiscard]] inline bool isBatchFunctionActive() noexcept {
    return g_batch_fun != nullptr;
}

// === Global State Management Functions ===

/**
 * @brief Reset all global state to default values.
 *
 * Useful for cleanup or testing scenarios. Thread-safe as all variables
 * are thread-local.
 */
inline void resetGlobalState() noexcept {
    g_reg_fun = nullptr;
    g_batch_fun = nullptr;
    g_batch_execute = false;
}

} // namespace reaction
