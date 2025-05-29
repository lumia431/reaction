#pragma once

#include <functional>

namespace reaction {

// Trigger mode that always returns true
struct AlwaysTrig {
    // Always triggers regardless of the arguments
    bool checkTrigger() {
        return true; // Always triggers
    }
};

// Trigger mode that triggers based on value change
struct ChangeTrig {
    // Only triggers if the provided boolean argument is true
    bool checkTrigger() {
        return m_changed; // All arguments must be true for the trigger to occur
    }

    void setChanged(bool changed) {
        m_changed = changed;
    }

private:
    bool m_changed = true;
};

// Trigger mode based on a threshold
struct FilterTrig {
    // Set the threshold function to determine whether to trigger based on arguments
    template <typename F, typename... A>
    void filter(F &&f, A &&...args) {
        m_filterFun = createFun(std::forward<F>(f), std::forward<A>(args)...);
    }

    // Check whether the trigger condition is met using the threshold function
    bool checkTrigger() {
        return std::invoke(m_filterFun); // Calls the threshold function to determine trigger condition
    }

private:
    template <typename F, typename... A>
    auto createFun(F &&f, A &&...args) {
        return [f = std::forward<F>(f), ... args = args.getPtr()]() {
            return std::invoke(f, args->get()...);
        };
    }

    // Function to evaluate whether the threshold condition is met
    std::function<bool()> m_filterFun = []() { return true; }; // Default threshold function always returns true
};

} // namespace reaction