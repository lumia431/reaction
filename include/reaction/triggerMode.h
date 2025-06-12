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
 * @brief Trigger policy that always triggers.
 *
 * The checkTrig() function always returns true,
 * indicating the reactive node should always trigger.
 */
struct AlwaysTrig {
    /**
     * @brief Always returns true to trigger.
     * @return true
     */
    bool checkTrig() {
        return true;
    }
};

/**
 * @brief Trigger policy that triggers only if a change flag is set.
 *
 * The flag can be controlled by setChanged().
 */
struct ChangeTrig {
public:
    /**
     * @brief Check if the trigger condition is met.
     * @return true if the change flag is set, false otherwise.
     */
    bool checkTrig() {
        return m_changed;
    }

    /**
     * @brief Set the internal change flag.
     * @param changed New value of the change flag.
     */
    void setChanged(bool changed) {
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
 * to have a getWeak() method returning a weak_ptr-like object.
 */
struct FilterTrig {
public:
    /**
     * @brief Set the filter function used to determine triggering.
     *
     * The filter function is created by binding the provided callable and arguments,
     * where arguments are expected to have getWeak() method returning weak pointers.
     *
     * @tparam F Callable type.
     * @tparam A Argument types.
     * @param f Callable object.
     * @param args Arguments to bind (expected to support getWeak()).
     */
    template <typename F, typename... A>
    void filter(F &&f, A &&...args) {
        m_filterFun = createFun(std::forward<F>(f), std::forward<A>(args)...);
    }

    /**
     * @brief Invoke the stored filter function to determine trigger condition.
     * @return true if filter passes, false otherwise.
     */
    bool checkTrig() {
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
     * @param args Arguments to bind (expected to support getWeak()).
     * @return A lambda function returning bool.
     */
    template <typename F, typename... A>
    auto createFun(F &&f, A &&...args) {
        // Capture f by forwarding, and capture each args by getting weak pointer
        return [f = std::forward<F>(f), ... args = args.getWeak()]() {
            // Lock each weak pointer and call f with dereferenced values
            return std::invoke(f, args.lock()->get()...);
        };
    }

    std::function<bool()> m_filterFun = []() { return true; }; ///< Default filter always returns true.
};

} // namespace reaction
