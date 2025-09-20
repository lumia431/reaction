/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <functional>

namespace reaction {

/**
 * @brief Base template for trigger policies with CRTP optimization.
 *
 * @tparam Derived The derived trigger type
 */
template<typename Derived>
struct TriggerBase {
    [[nodiscard]] constexpr bool checkTrig() const noexcept {
        return static_cast<const Derived*>(this)->checkTrigImpl();
    }
};

/**
 * @brief Optimized trigger policy that always triggers.
 *
 * Uses compile-time constant for maximum optimization.
 */
struct AlwaysTrig : TriggerBase<AlwaysTrig> {
    /**
     * @brief Always returns true to trigger (compile-time constant).
     * @return true
     */
    [[nodiscard]] static constexpr bool checkTrigImpl() noexcept {
        return true;
    }
};

/**
 * @brief Enhanced trigger policy that triggers only if a change flag is set.
 *
 * Optimized for performance with better memory layout.
 */
struct ChangeTrig : TriggerBase<ChangeTrig> {
public:
    /**
     * @brief Check if the trigger condition is met.
     * @return true if the change flag is set, false otherwise.
     */
    [[nodiscard]] constexpr bool checkTrigImpl() const noexcept {
        return m_changed;
    }

    /**
     * @brief Set the internal change flag.
     * @param changed New value of the change flag.
     */
    constexpr void setChanged(bool changed) noexcept {
        m_changed = changed;
    }

private:
    bool m_changed = true; ///< Internal flag indicating whether a change occurred.
};

/**
 * @brief Trigger policy that uses a user-provided filter function.
 *
 * The filter function is stored internally as a std::function<bool()>.
 * It is created from a callable and its arguments, where arguments are expected
 * to have a getPtr() method returning a shared_ptr-like object.
 */
struct FilterTrig {
public:
    /**
     * @brief Set the filter function used to determine triggering.
     *
     * The filter function is created by binding the provided callable and arguments,
     * where arguments are expected to have getPtr() method returning shared_ptr pointers.
     *
     * @tparam F Callable type.
     * @tparam A Argument types.
     * @param f Callable object.
     * @param args Arguments to bind (expected to support getPtr()).
     */
    template <typename F, typename... A>
    void filter(F &&f, A &&...args) {
        m_filterFun = createFun(std::forward<F>(f), std::forward<A>(args)...);
    }

    /**
     * @brief Invoke the stored filter function to determine trigger condition.
     * @return true if filter passes, false otherwise.
     */
    [[nodiscard]] bool checkTrig() const noexcept {
        return std::invoke(m_filterFun);
    }

private:
    /**
     * @brief Helper to create the stored filter function by binding callable and weak-locked arguments.
     *
     * Uses C++17 fold expressions to capture each argument's weak pointer lock and get its value.
     *
     * @tparam F Callable type.
     * @tparam A Argument types.
     * @param f Callable object.
     * @param args Arguments to bind (expected to support getPtr()).
     * @return A lambda function returning bool.
     */
    template <typename F, typename... A>
    auto createFun(F &&f, A &&...args) {
        // Capture f by forwarding, and capture each args by getting weak pointer
        return [f = std::forward<F>(f), ... args = args.getPtr()]() {
            // Lock each weak pointer and call f with dereferenced values
            return std::invoke(f, args->get()...);
        };
    }

    std::function<bool()> m_filterFun = []() { return true; }; ///< Default filter always returns true.
};

} // namespace reaction
