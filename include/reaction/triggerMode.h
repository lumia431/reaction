#ifndef REACTION_TRIGGERMODE_H
#define REACTION_TRIGGERMODE_H

namespace reaction {

// Creates a functor to get the value from the function and arguments
template <typename F, typename... A>
inline auto createGetFunRef(F &&f, A &&...args) {
    return [f = std::forward<F>(f), &...args = *args]() mutable {
        return std::invoke(f, args.get()...); // Invokes function f with arguments' values
    };
}

// Creates a functor to get the updated value from the function and arguments
template <typename F, typename... A>
inline auto createUpdateFunRef(F &&f, A &&...args) {
    return [f = std::forward<F>(f), &...args = *args]() mutable {
        return std::invoke(f, args.getUpdate()...); // Invokes function f with updated values
    };
}

// Trigger mode that always returns true
struct AlwaysTrigger {
    // Always triggers regardless of the arguments
    bool checkTrigger(bool trigger = true) {
        return true; // Always triggers
    }
};

// Trigger mode that triggers based on value change
struct ValueChangeTrigger {
    // Only triggers if the provided boolean argument is true
    bool checkTrigger(bool trigger = true) {
        return trigger; // All arguments must be true for the trigger to occur
    }
};

// Trigger mode based on a threshold, supports repeat dependencies
struct ThresholdTrigger {
    // Set the threshold function to determine whether to trigger based on arguments
    template <typename F, ArgSizeOverZeroCC... A>
    void setThreshold(F &&f, A &&...args) {
        // Use different function references based on whether the repeat dependency is set
        if (m_repeatDependent) {
            m_thresholdFun = createUpdateFunRef(std::forward<F>(f), std::forward<A>(args)...);
        } else {
            m_thresholdFun = createGetFunRef(std::forward<F>(f), std::forward<A>(args)...);
        }
    }

    // Check whether the trigger condition is met using the threshold function
    bool checkTrigger(bool trigger = true) {
        return std::invoke(m_thresholdFun); // Calls the threshold function to determine trigger condition
    }

    // Set whether the trigger depends on repeated conditions
    void setRepeatDependent(bool repeat) {
        m_repeatDependent = repeat;
    }

private:
    // Function to evaluate whether the threshold condition is met
    std::function<bool()> m_thresholdFun = []() { return true; }; // Default threshold function always returns true
    bool m_repeatDependent = false;                               // Flag indicating if the trigger depends on repeat conditions
};

} // namespace reaction

#endif // REACTION_TRIGGERMODE_H
