/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "memory_config.h"
#include <atomic>
#include <thread>

namespace reaction::memory {

/**
 * @brief Thread-local stack usage monitor for SBO safety.
 *
 * This class tracks the amount of stack memory used by SBO allocations
 * to prevent stack overflow. Each thread maintains its own usage counter.
 */
class StackMonitor {
private:
    thread_local static std::atomic<size_t> current_usage;
    thread_local static bool monitoring_enabled;

public:
    /**
     * @brief Initialize stack monitoring for current thread.
     */
    static void initialize() noexcept {
        current_usage.store(0, std::memory_order_relaxed);
        monitoring_enabled = true;
    }

    /**
     * @brief Get current stack usage by SBO allocations.
     */
    static size_t getCurrentUsage() noexcept {
        return current_usage.load(std::memory_order_relaxed);
    }

    /**
     * @brief Check if we can safely allocate the given size on stack.
     */
    template<size_t Size>
    static bool canAllocate() noexcept {
        if (!monitoring_enabled) {
            return Size <= SBOConfig::CONDITIONAL_SBO_SIZE;
        }

        size_t current = current_usage.load(std::memory_order_relaxed);
        return (current + Size) <= SBOConfig::MAX_SBO_STACK_USAGE;
    }

    /**
     * @brief Check if we can safely allocate the given size on stack (runtime version).
     */
    static bool canAllocate(size_t size) noexcept {
        if (!monitoring_enabled) {
            return size <= SBOConfig::CONDITIONAL_SBO_SIZE;
        }

        size_t current = current_usage.load(std::memory_order_relaxed);
        return (current + size) <= SBOConfig::MAX_SBO_STACK_USAGE;
    }

    /**
     * @brief RAII guard for stack usage tracking.
     *
     * Automatically increments usage on construction and decrements on destruction.
     * Reports whether the allocation was successful.
     */
    template<size_t Size>
    class StackGuard {
    private:
        bool allocated;

    public:
        StackGuard() noexcept : allocated(false) {
            if (canAllocate<Size>()) {
                current_usage.fetch_add(Size, std::memory_order_relaxed);
                allocated = true;
            }
        }

        explicit StackGuard(size_t size) noexcept : allocated(false) {
            if (canAllocate(size)) {
                current_usage.fetch_add(size, std::memory_order_relaxed);
                allocated = true;
            }
        }

        ~StackGuard() noexcept {
            if (allocated) {
                current_usage.fetch_sub(Size, std::memory_order_relaxed);
            }
        }

        // Non-copyable, non-movable
        StackGuard(const StackGuard&) = delete;
        StackGuard& operator=(const StackGuard&) = delete;
        StackGuard(StackGuard&&) = delete;
        StackGuard& operator=(StackGuard&&) = delete;

        /**
         * @brief Check if the stack allocation was successful.
         */
        bool success() const noexcept {
            return allocated;
        }

        /**
         * @brief Explicit conversion to bool for convenience.
         */
        explicit operator bool() const noexcept {
            return allocated;
        }
    };

    /**
     * @brief Get usage statistics for debugging.
     */
    struct UsageStats {
        size_t current_usage;
        size_t max_allowed;
        double usage_percentage;
        bool near_limit;
    };

    static UsageStats getStats() noexcept {
        size_t current = current_usage.load(std::memory_order_relaxed);
        size_t max_allowed = SBOConfig::MAX_SBO_STACK_USAGE;
        double percentage = (static_cast<double>(current) / max_allowed) * 100.0;
        bool near_limit = current >= SBOConfig::STACK_USAGE_WARNING;

        return UsageStats{
            .current_usage = current,
            .max_allowed = max_allowed,
            .usage_percentage = percentage,
            .near_limit = near_limit
        };
    }

    /**
     * @brief Enable or disable monitoring for current thread.
     */
    static void setMonitoringEnabled(bool enabled) noexcept {
        monitoring_enabled = enabled;
        if (!enabled) {
            current_usage.store(0, std::memory_order_relaxed);
        }
    }

    /**
     * @brief Check if monitoring is enabled for current thread.
     */
    static bool isMonitoringEnabled() noexcept {
        return monitoring_enabled;
    }

private:
    template<size_t Size>
    friend class StackGuard;
};

// Thread-local storage definitions - use inline to avoid multiple definition
inline thread_local std::atomic<size_t> StackMonitor::current_usage{0};
inline thread_local bool StackMonitor::monitoring_enabled{false};

} // namespace reaction::memory