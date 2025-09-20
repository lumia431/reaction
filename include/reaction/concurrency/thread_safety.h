/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <atomic>
#include <shared_mutex>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <memory>

// Compile-time configuration for forcing thread safety
#ifndef REACTION_FORCE_THREAD_SAFETY
#define REACTION_FORCE_THREAD_SAFETY 0
#endif

// Thread safety mode detection
#ifndef REACTION_THREAD_SAFETY_MODE
#if REACTION_FORCE_THREAD_SAFETY
#define REACTION_THREAD_SAFETY_MODE 1  // Always thread-safe
#else
#define REACTION_THREAD_SAFETY_MODE 0  // Auto-detect
#endif
#endif

namespace reaction {

/**
 * @brief Thread safety manager for automatic multi-threading detection and optimization.
 *
 * This singleton class manages thread safety behavior throughout the reactive system.
 * It automatically detects multi-threading scenarios and enables thread safety measures
 * only when needed, providing zero-overhead for single-threaded applications.
 */
class ThreadSafetyManager {
public:
    /**
     * @brief Get the singleton instance.
     * @return ThreadSafetyManager& Reference to the singleton instance.
     */
    static ThreadSafetyManager& getInstance() noexcept {
        static ThreadSafetyManager instance;
        return instance;
    }

    /**
     * @brief Check if thread safety is currently enabled.
     * @return true if thread safety is enabled, false otherwise.
     */
    bool isThreadSafetyEnabled() const noexcept {
        return m_threadSafetyEnabled.load(std::memory_order_acquire);
    }

    /**
     * @brief Force enable thread safety mode.
     * Once enabled, cannot be disabled during runtime.
     */
    void enableThreadSafety() noexcept {
        bool expected = false;
        if (m_threadSafetyEnabled.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            // First time enabling - log or perform any initialization
        }
    }

    /**
     * @brief Register the current thread as active.
     * Automatically enables thread safety if multiple threads are detected.
     */
    void registerThread() noexcept {
        std::thread::id currentId = std::this_thread::get_id();

        // Fast path for already known single thread
        if (!m_threadSafetyEnabled.load(std::memory_order_acquire)) {
            std::thread::id expected = std::thread::id{};
            if (m_firstThreadId.compare_exchange_strong(expected, currentId, std::memory_order_acq_rel)) {
                // First thread registered
                return;
            } else if (expected == currentId) {
                // Same thread as before
                return;
            } else {
                // Different thread detected - enable thread safety
                enableThreadSafety();
            }
        }
    }

    /**
     * @brief Get the number of active threads (for debugging/monitoring).
     * @return size_t Number of threads that have been registered.
     */
    size_t getThreadCount() const noexcept {
        if (!m_threadSafetyEnabled.load(std::memory_order_acquire)) {
            return m_firstThreadId.load(std::memory_order_acquire) != std::thread::id{} ? 1 : 0;
        }
        return 2; // We know at least 2 threads exist if thread safety is enabled
    }

private:
    ThreadSafetyManager() = default;
    ~ThreadSafetyManager() = default;
    ThreadSafetyManager(const ThreadSafetyManager&) = delete;
    ThreadSafetyManager& operator=(const ThreadSafetyManager&) = delete;

    std::atomic<bool> m_threadSafetyEnabled{REACTION_THREAD_SAFETY_MODE};
    std::atomic<std::thread::id> m_firstThreadId{};
};

/**
 * @brief RAII thread registration guard.
 * Automatically registers the current thread when constructed.
 */
class ThreadRegistrationGuard {
public:
    ThreadRegistrationGuard() {
        ThreadSafetyManager::getInstance().registerThread();
    }
};

// === Conditional Mutex Abstractions with Template Optimization ===

/**
 * @brief Unified conditional mutex wrapper with template specialization.
 *
 * This template automatically adapts between thread-safe and single-threaded modes
 * with compile-time optimization for better performance.
 *
 * @tparam MutexType The underlying mutex type (std::mutex, std::shared_mutex, etc.)
 */
template<typename MutexType>
class ConditionalMutexWrapper {
public:
    /// @brief Acquire lock if thread safety is enabled.
    void lock() noexcept(noexcept(std::declval<MutexType>().lock())) {
        if (ThreadSafetyManager::getInstance().isThreadSafetyEnabled()) [[likely]] {
            m_mutex.lock();
        }
    }

    /// @brief Release lock if thread safety is enabled.
    void unlock() noexcept(noexcept(std::declval<MutexType>().unlock())) {
        if (ThreadSafetyManager::getInstance().isThreadSafetyEnabled()) [[likely]] {
            m_mutex.unlock();
        }
    }

    /// @brief Try to acquire lock if thread safety is enabled.
    [[nodiscard]] bool try_lock() noexcept(noexcept(std::declval<MutexType>().try_lock())) {
        if (ThreadSafetyManager::getInstance().isThreadSafetyEnabled()) [[likely]] {
            return m_mutex.try_lock();
        }
        return true; // Always succeed when thread safety is disabled
    }

    // Shared mutex operations (SFINAE-enabled for types that support them)
    template<typename T = MutexType>
    auto lock_shared() noexcept(noexcept(std::declval<T>().lock_shared()))
        -> decltype(std::declval<T>().lock_shared(), void()) {
        if (ThreadSafetyManager::getInstance().isThreadSafetyEnabled()) [[likely]] {
            m_mutex.lock_shared();
        }
    }

    template<typename T = MutexType>
    auto unlock_shared() noexcept(noexcept(std::declval<T>().unlock_shared()))
        -> decltype(std::declval<T>().unlock_shared(), void()) {
        if (ThreadSafetyManager::getInstance().isThreadSafetyEnabled()) [[likely]] {
            m_mutex.unlock_shared();
        }
    }

    template<typename T = MutexType>
    auto try_lock_shared() noexcept(noexcept(std::declval<T>().try_lock_shared()))
        -> decltype(std::declval<T>().try_lock_shared()) {
        if (ThreadSafetyManager::getInstance().isThreadSafetyEnabled()) [[likely]] {
            return m_mutex.try_lock_shared();
        }
        return true; // Always succeed when thread safety is disabled
    }

private:
    MutexType m_mutex;
};

/**
 * @brief Conditional shared mutex that provides no-op when thread safety is disabled.
 */
using ConditionalSharedMutex = ConditionalMutexWrapper<std::shared_mutex>;

/**
 * @brief Conditional mutex that provides no-op when thread safety is disabled.
 */
using ConditionalMutex = ConditionalMutexWrapper<std::mutex>;

// === Optimized Lock Guard Abstractions ===

/**
 * @brief Template-based RAII lock guard generator.
 *
 * @param GuardName The name of the guard class
 * @param LockMethod The locking method name
 * @param UnlockMethod The unlocking method name
 */
#define REACTION_DEFINE_LOCK_GUARD(GuardName, LockMethod, UnlockMethod) \
    template<typename Mutex> \
    class GuardName { \
    public: \
        explicit GuardName(Mutex& mutex) noexcept(noexcept(mutex.LockMethod())) \
            : m_mutex(mutex) { \
            m_mutex.LockMethod(); \
        } \
        \
        ~GuardName() noexcept(noexcept(m_mutex.UnlockMethod())) { \
            m_mutex.UnlockMethod(); \
        } \
        \
        GuardName(const GuardName&) = delete; \
        GuardName& operator=(const GuardName&) = delete; \
        GuardName(GuardName&&) = delete; \
        GuardName& operator=(GuardName&&) = delete; \
        \
    private: \
        Mutex& m_mutex; \
    }

// Generate optimized lock guards
REACTION_DEFINE_LOCK_GUARD(ConditionalLockGuard, lock, unlock);
REACTION_DEFINE_LOCK_GUARD(ConditionalSharedLockGuard, lock_shared, unlock_shared);

/**
 * @brief RAII shared lock guard for conditional shared mutex.
 */
template<typename Mutex>
using ConditionalSharedLock = ConditionalSharedLockGuard<Mutex>;

/**
 * @brief RAII unique lock guard for conditional mutex.
 */
template<typename Mutex>
using ConditionalUniqueLock = ConditionalLockGuard<Mutex>;

// Helper macros for thread registration with better performance
#define REACTION_REGISTER_THREAD() \
    do { \
        static thread_local reaction::ThreadRegistrationGuard thread_guard_##__COUNTER__; \
        (void)thread_guard_##__COUNTER__; \
    } while(0)

// Cleanup macro to avoid pollution
#undef REACTION_DEFINE_LOCK_GUARD

} // namespace reaction