/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "memory_config.h"
#include "stack_monitor.h"
#include "reaction/concurrency/thread_manager.h"
#include "reaction/core/exception.h"
#include "reaction/core/observer_node.h"
#include "reaction/core/concept.h"
#include "reaction/graph/batch.h"  // For g_batch_execute
#include <iostream>
#include <memory>
#include <mutex>
#include <new>
#include <shared_mutex>
#include <type_traits>

// Forward declare Void to avoid circular dependency
namespace reaction {
    struct Void;  // Forward declaration only
}

namespace reaction::memory {

/**
 * @brief Small Buffer Optimized Resource storage.
 *
 * This class provides automatic stack/heap allocation based on type size and runtime conditions.
 * Small objects are stored inline to avoid heap allocation overhead.
 *
 * @tparam Type The type of resource to store.
 */
template<typename Type>
class SBOResource : public ObserverNode {
private:
    // Storage decision logic
    static constexpr bool always_sbo = SBOTraits<Type>::always_sbo;
    static constexpr bool conditional_sbo = SBOTraits<Type>::conditional_sbo;
    static constexpr bool force_heap = SBOTraits<Type>::force_heap;

    // Union for storage flexibility
    union Storage {
        alignas(Type) char buffer[sizeof(Type)];  // Stack storage
        std::unique_ptr<Type> heap_ptr;           // Heap storage

        Storage() noexcept {} // No initialization - manual lifetime management
        ~Storage() noexcept {} // Manual lifetime management
    } storage;

    bool is_sbo_storage;  // Track which storage is active
    bool is_initialized;  // Track initialization state

public:
    /**
     * @brief Default constructor with null state.
     */
    SBOResource() : is_sbo_storage(false), is_initialized(false) {
        // Initialize heap_ptr explicitly
        new(&storage.heap_ptr) std::unique_ptr<Type>(nullptr);
    }

    /**
     * @brief Constructor with forwarded arguments.
     */
    template<typename T>
        requires(!std::is_same_v<std::remove_cvref_t<T>, SBOResource<Type>>)
    SBOResource(T&& t) {
        // Determine storage strategy first
        if constexpr (SBOTraits<Type>::always_sbo) {
            // Always use SBO - no heap_ptr needed
            new(storage.buffer) Type(std::forward<T>(t));
            is_sbo_storage = true;
            is_initialized = true;
        } else if constexpr (SBOTraits<Type>::conditional_sbo) {
            // Conditional SBO based on stack usage
            StackMonitor::StackGuard<sizeof(Type)> guard;
            if (guard.success()) {
                // Use SBO
                new(storage.buffer) Type(std::forward<T>(t));
                is_sbo_storage = true;
                is_initialized = true;
            } else {
                // Use heap allocation
                new(&storage.heap_ptr) std::unique_ptr<Type>(std::make_unique<Type>(std::forward<T>(t)));
                is_sbo_storage = false;
                is_initialized = true;
            }
        } else {
            // Force heap allocation
            new(&storage.heap_ptr) std::unique_ptr<Type>(std::make_unique<Type>(std::forward<T>(t)));
            is_sbo_storage = false;
            is_initialized = true;
        }
    }

    /**
     * @brief Destructor with proper cleanup.
     */
    ~SBOResource() noexcept {
        destroy();
    }

    // Non-copyable to match original Resource behavior
    SBOResource(const SBOResource&) = delete;
    SBOResource& operator=(const SBOResource&) = delete;

    /**
     * @brief Get a copy of the stored value (thread-safe).
     */
    [[nodiscard]] Type getValue() const {
        ConditionalSharedLock<ConditionalSharedMutex> lock(m_resourceMutex);
        if (!is_initialized) {
            REACTION_THROW_RESOURCE_NOT_INITIALIZED("SBOResource");
        }

        // Fast path for always_sbo types - eliminate runtime branch
        if constexpr (always_sbo) {
            return *reinterpret_cast<const Type*>(storage.buffer);
        } else {
            // Runtime branch for conditional_sbo types
            if (is_sbo_storage) {
                return *reinterpret_cast<const Type*>(storage.buffer);
            } else {
                return *storage.heap_ptr;
            }
        }
    }

    /**
     * @brief Update the stored value.
     *
     * @return true if the value changed, false otherwise.
     */
    template<typename T>
    bool updateValue(T&& t) noexcept {
        REACTION_REGISTER_THREAD();
        ConditionalUniqueLock<ConditionalSharedMutex> lock(m_resourceMutex);

        bool changed = true;

        if (!is_initialized) {
            // First initialization
            initializeWithImpl(std::forward<T>(t));
        } else {
            // Update existing value
            if constexpr (ComparableType<Type>) {
                // Create local copy to avoid data tearing during comparison
                Type newValue(std::forward<T>(t));

                // Fast path for always_sbo types - eliminate runtime branch
                if constexpr (always_sbo) {
                    Type* current = reinterpret_cast<Type*>(storage.buffer);
                    changed = (*current != newValue);
                    if (changed) {
                        *current = std::move(newValue);
                    }
                } else {
                    // Runtime branch for conditional_sbo types
                    if (is_sbo_storage) {
                        Type* current = reinterpret_cast<Type*>(storage.buffer);
                        changed = (*current != newValue);
                        if (changed) {
                            *current = std::move(newValue);
                        }
                    } else {
                        changed = (*storage.heap_ptr != newValue);
                        if (changed) {
                            *storage.heap_ptr = std::move(newValue);
                        }
                    }
                }
            } else {
                // Non-comparable types: always assume changed
                // Fast path for always_sbo types - eliminate runtime branch
                if constexpr (always_sbo) {
                    *reinterpret_cast<Type*>(storage.buffer) = std::forward<T>(t);
                } else {
                    // Runtime branch for conditional_sbo types
                    if (is_sbo_storage) {
                        *reinterpret_cast<Type*>(storage.buffer) = std::forward<T>(t);
                    } else {
                        *storage.heap_ptr = std::forward<T>(t);
                    }
                }
            }
        }

        return changed;
    }

    /**
     * @brief Get raw pointer to the stored value.
     */
    [[nodiscard]] Type* getRawPtr() const {
        ConditionalSharedLock<ConditionalSharedMutex> lock(m_resourceMutex);
        if (!is_initialized) {
            REACTION_THROW_NULL_POINTER("SBOResource not initialized");
        }

        // Fast path for always_sbo types - eliminate runtime branch
        if constexpr (always_sbo) {
            return reinterpret_cast<Type*>(const_cast<char*>(storage.buffer));
        } else {
            // Runtime branch for conditional_sbo types
            if (is_sbo_storage) {
                return reinterpret_cast<Type*>(const_cast<char*>(storage.buffer));
            } else {
                return storage.heap_ptr.get();
            }
        }
    }

    /**
     * @brief Check if this resource is using SBO storage.
     */
    [[nodiscard]] bool isUsingSBO() const noexcept {
        return is_sbo_storage;
    }

    /**
     * @brief Get storage information for debugging.
     */
    struct StorageInfo {
        bool using_sbo;
        bool initialized;
        size_t type_size;
        size_t stack_usage;
    };

    [[nodiscard]] StorageInfo getStorageInfo() const noexcept {
        return StorageInfo{
            .using_sbo = is_sbo_storage,
            .initialized = is_initialized,
            .type_size = sizeof(Type),
            .stack_usage = is_sbo_storage ? sizeof(Type) : 0
        };
    }

    /**
     * @brief Generic atomic operation helper.
     *
     * @tparam F Operation function type (should take Type& and return bool indicating if changed).
     * @param operation The operation to perform on the value.
     * @param alwaysChanged If true, always consider the operation as changing the value.
     */
    template <typename F>
    void atomicOperation(F &&operation, bool alwaysChanged = false) {
        REACTION_REGISTER_THREAD();
        bool changed = false;
        {
            ConditionalUniqueLock<ConditionalSharedMutex> lock(m_resourceMutex);
            try {
                // Direct access to avoid lock recursion
                if (!is_initialized) {
                    REACTION_THROW_RESOURCE_NOT_INITIALIZED("SBOResource");
                }

                if (is_sbo_storage) {
                    Type* ptr = reinterpret_cast<Type*>(const_cast<char*>(storage.buffer));
                    changed = operation(*ptr);
                } else {
                    changed = operation(*storage.heap_ptr);
                }

                if (alwaysChanged) {
                    changed = true;
                }
            } catch (const std::runtime_error&) {
                // Resource not initialized, treat as no change
                changed = false;
            }
        } // Lock is automatically released here

        // Trigger notifications like ReactImpl does
        if (!g_batch_execute && changed) {
            this->notify(true);
        }
    }

private:
    /**
     * @brief Initialize storage with given value.
     */
    template<typename T>
    void initializeWith(T&& t) {
        try {
            initializeWithImpl(std::forward<T>(t));
        } catch (...) {
            is_initialized = false;
            is_sbo_storage = false;
            throw;
        }
    }

    /**
     * @brief Internal initialization implementation.
     */
    template<typename T>
    void initializeWithImpl(T&& t) {
        if constexpr (always_sbo) {
            // Always use SBO for small types
            if (is_initialized && !is_sbo_storage) {
                // Destroy heap_ptr if it was active from a previous state
                storage.heap_ptr.~unique_ptr<Type>();
            } else if (!is_initialized) {
                // First time: destroy the default-constructed heap_ptr
                storage.heap_ptr.~unique_ptr<Type>();
            }
            new(storage.buffer) Type(std::forward<T>(t));
            is_sbo_storage = true;
            is_initialized = true;
        } else if constexpr (conditional_sbo) {
            // Conditional SBO based on stack usage
            StackMonitor::StackGuard<sizeof(Type)> guard;
            if (guard.success()) {
                // Use SBO
                if (is_initialized && !is_sbo_storage) {
                    // Destroy heap_ptr if it was active from a previous state
                    storage.heap_ptr.~unique_ptr<Type>();
                } else if (!is_initialized) {
                    // First time: destroy the default-constructed heap_ptr
                    storage.heap_ptr.~unique_ptr<Type>();
                }
                new(storage.buffer) Type(std::forward<T>(t));
                is_sbo_storage = true;
                is_initialized = true;
            } else {
                // Use heap allocation
                if (is_initialized && is_sbo_storage) {
                    // Destroy buffer content and re-construct heap_ptr
                    reinterpret_cast<Type*>(storage.buffer)->~Type();
                    new(&storage.heap_ptr) std::unique_ptr<Type>();
                }
                // heap_ptr is already available - either from constructor or re-construction above
                storage.heap_ptr = std::make_unique<Type>(std::forward<T>(t));
                is_sbo_storage = false;
                is_initialized = true;
            }
        } else {
            // Force heap allocation
            if (is_initialized && is_sbo_storage) {
                // Destroy buffer content and re-construct heap_ptr
                reinterpret_cast<Type*>(storage.buffer)->~Type();
                new(&storage.heap_ptr) std::unique_ptr<Type>();
            }
            // heap_ptr is already available - either from constructor or re-construction above
            storage.heap_ptr = std::make_unique<Type>(std::forward<T>(t));
            is_sbo_storage = false;
            is_initialized = true;
        }
    }

    /**
     * @brief Destroy stored object and cleanup.
     */
    void destroy() noexcept {
        if (is_initialized) {
            if (is_sbo_storage) {
                // Destroy the object in buffer
                reinterpret_cast<Type*>(storage.buffer)->~Type();
            } else {
                // Destroy heap_ptr
                storage.heap_ptr.~unique_ptr<Type>();
            }
            is_initialized = false;
            is_sbo_storage = false;
        } else {
            // For uninitialized default-constructed objects, destroy heap_ptr
            storage.heap_ptr.~unique_ptr<Type>();
        }
    }

protected:
    mutable ConditionalSharedMutex m_resourceMutex; ///< Thread-safe access mutex
};

} // namespace reaction::memory