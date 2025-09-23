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

// === Global State Variables for Reactive System ===

/**
 * @brief Thread-local function pointer for dependency registration.
 *
 * When set, this function is called whenever a reactive value is accessed,
 * allowing automatic dependency tracking during expression evaluation.
 * Thread-local to ensure isolation between threads for reactive computations.
 */
inline thread_local std::function<void(const NodePtr &)> g_reg_fun = nullptr;

/**
 * @brief Thread-local function pointer for batch operation tracking.
 *
 * When set, this function is called when reactive values are modified
 * during batch operations, allowing deferred notification handling.
 * Thread-local to enable per-thread batch operations.
 */
inline thread_local std::function<void(const NodePtr &)> g_batch_fun = nullptr;

/**
 * @brief Thread-local flag indicating whether a batch operation is currently executing.
 *
 * When true, reactive notifications are deferred until the batch completes.
 * Thread-local to allow independent batch operations per thread.
 */
inline thread_local bool g_batch_execute = false;

// === Global State Access Functions ===

/**
 * @brief Check if dependency registration is active.
 * @return true if dependency registration function is set.
 */
inline bool isDependencyTrackingActive() noexcept {
    return g_reg_fun != nullptr;
}

/**
 * @brief Check if batch operation is active.
 * @return true if batch operation is currently executing.
 */
inline bool isBatchActive() noexcept {
    return g_batch_execute;
}

/**
 * @brief Check if batch function tracking is active.
 * @return true if batch function is set.
 */
inline bool isBatchFunctionActive() noexcept {
    return g_batch_fun != nullptr;
}

/**
 * @brief Reset all global state to default values.
 *
 * Useful for cleanup or testing scenarios.
 */
inline void resetGlobalState() noexcept {
    g_reg_fun = nullptr;
    g_batch_fun = nullptr;
    g_batch_execute = false;
}

} // namespace reaction
