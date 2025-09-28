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

// === Thread-Local Global State Variables ===

inline thread_local std::function<void(const NodePtr &)> g_reg_fun = nullptr;
inline thread_local std::function<void(const NodePtr &)> g_batch_fun = nullptr;
inline thread_local bool g_batch_execute = false;

// === Generic ScopedValue ===

/**
 * @brief RAII guard for temporarily setting a variable.
 * Restores the old value automatically on destruction.
 *
 * @tparam T Type of the variable
 */
template <typename T>
class ScopedValue {
public:
    ScopedValue(T &target, T newValue)
        : ref_(target), old_(target) {
        ref_ = std::move(newValue);
    }

    ~ScopedValue() {
        ref_ = std::move(old_);
    }

    ScopedValue(const ScopedValue &) = delete;
    ScopedValue &operator=(const ScopedValue &) = delete;

private:
    T &ref_;
    T old_;
};

// === Type Aliases with Pre-bound Targets ===

struct RegFunGuard : ScopedValue<std::function<void(const NodePtr &)>> {
    explicit RegFunGuard(std::function<void(const NodePtr &)> fn)
        : ScopedValue(g_reg_fun, std::move(fn)) {}
};

struct BatchFunGuard : ScopedValue<std::function<void(const NodePtr &)>> {
    explicit BatchFunGuard(std::function<void(const NodePtr &)> fn)
        : ScopedValue(g_batch_fun, std::move(fn)) {}
};

struct BatchExeGuard : ScopedValue<bool> {
    explicit BatchExeGuard(bool flag)
        : ScopedValue(g_batch_execute, flag) {}
};

// === Global State Query Functions ===

[[nodiscard]] inline bool isDependencyTrackingActive() noexcept {
    return g_reg_fun != nullptr;
}

[[nodiscard]] inline bool isBatchActive() noexcept {
    return g_batch_execute;
}

[[nodiscard]] inline bool isBatchFunctionActive() noexcept {
    return g_batch_fun != nullptr;
}

// === Reset Function ===

inline void resetGlobalState() noexcept {
    g_reg_fun = nullptr;
    g_batch_fun = nullptr;
    g_batch_execute = false;
}

} // namespace reaction
