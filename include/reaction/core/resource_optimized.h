/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/concurrency/thread_manager.h"
#include "reaction/core/exception.h"
#include "reaction/core/observer_node.h"
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

namespace reaction {

/**
 * @brief Small Buffer Optimization threshold in bytes.
 * Objects smaller than this will be stored on the stack.
 */
constexpr size_t SBO_THRESHOLD = 32;

/**
 * @brief Type trait to determine if a type should use Small Buffer Optimization.
 * 
 * @tparam Type The type to check.
 */
template <typename Type>
constexpr bool should_use_sbo_v = sizeof(Type) <= SBO_THRESHOLD && 
                                  std::is_trivially_destructible_v<Type> &&
                                  alignof(Type) <= alignof(std::max_align_t);

/**
 * @brief Optimized resource wrapper that uses Small Buffer Optimization (SBO).
 * 
 * For small types (typically <= 32 bytes), stores the value directly in the object
 * to avoid heap allocation. For larger types, falls back to std::unique_ptr.
 * 
 * @tparam Type The type of the resource to manage.
 */
template <typename Type>
class OptimizedResource : public ObserverNode {
private:
    static constexpr bool use_sbo = should_use_sbo_v<Type>;
    
public:
    /**
     * @brief Default constructor initializes with no value.
     */
    OptimizedResource() : m_initialized(false) {
        if constexpr (!use_sbo) {
            m_storage.heap_ptr = nullptr;
        }
    }

    /**
     * @brief Constructor that initializes the resource with a forwarded value.
     * 
     * @tparam T Type of the initialization argument.
     * @param t The initialization value for the resource.
     */
    template <typename T>
        requires(!std::is_same_v<std::remove_cvref_t<T>, OptimizedResource<Type>>)
    OptimizedResource(T &&t) : m_initialized(true) {
        if constexpr (use_sbo) {
            new (&m_storage.stack_storage) Type(std::forward<T>(t));
        } else {
            m_storage.heap_ptr = std::make_unique<Type>(std::forward<T>(t));
        }
    }

    /**
     * @brief Destructor that properly cleans up the resource.
     */
    ~OptimizedResource() {
        if (m_initialized) {
            if constexpr (use_sbo) {
                getStackPtr()->~Type();
            } else {
                m_storage.heap_ptr.reset();
            }
        }
    }

    // Delete copy constructor and assignment to avoid complexity
    OptimizedResource(const OptimizedResource &) = delete;
    OptimizedResource &operator=(const OptimizedResource &) = delete;

    /**
     * @brief Move constructor.
     */
    OptimizedResource(OptimizedResource &&other) noexcept : m_initialized(other.m_initialized) {
        if (m_initialized) {
            if constexpr (use_sbo) {
                if constexpr (std::is_move_constructible_v<Type>) {
                    new (&m_storage.stack_storage) Type(std::move(*other.getStackPtr()));
                } else {
                    new (&m_storage.stack_storage) Type(*other.getStackPtr());
                }
                other.getStackPtr()->~Type();
            } else {
                m_storage.heap_ptr = std::move(other.m_storage.heap_ptr);
            }
            other.m_initialized = false;
        }
    }

    /**
     * @brief Move assignment operator.
     */
    OptimizedResource &operator=(OptimizedResource &&other) noexcept {
        if (this != &other) {
            // Clean up current state
            if (m_initialized) {
                if constexpr (use_sbo) {
                    getStackPtr()->~Type();
                } else {
                    m_storage.heap_ptr.reset();
                }
            }

            // Move from other
            m_initialized = other.m_initialized;
            if (m_initialized) {
                if constexpr (use_sbo) {
                    if constexpr (std::is_move_constructible_v<Type>) {
                        new (&m_storage.stack_storage) Type(std::move(*other.getStackPtr()));
                    } else {
                        new (&m_storage.stack_storage) Type(*other.getStackPtr());
                    }
                    other.getStackPtr()->~Type();
                } else {
                    m_storage.heap_ptr = std::move(other.m_storage.heap_ptr);
                }
                other.m_initialized = false;
            }
        }
        return *this;
    }

    /**
     * @brief Get a copy of the managed resource value in a thread-safe manner.
     * 
     * @return Type Copy of the managed resource.
     */
    [[nodiscard]] Type getValue() const {
        ConditionalSharedLock<ConditionalSharedMutex> lock(m_resourceMutex);
        if (!m_initialized) {
            REACTION_THROW_RESOURCE_NOT_INITIALIZED("OptimizedResource");
        }
        
        if constexpr (use_sbo) {
            return *getStackPtr();
        } else {
            return *m_storage.heap_ptr;
        }
    }

    /**
     * @brief Update the managed resource with a new value.
     * 
     * @tparam T Type of the new value to update with.
     * @param t The new value.
     * @return true if the value changed.
     */
    template <typename T>
    bool updateValue(T &&t) noexcept {
        REACTION_REGISTER_THREAD();
        ConditionalUniqueLock<ConditionalSharedMutex> lock(m_resourceMutex);
        
        bool changed = true;
        
        if (!m_initialized) {
            if constexpr (use_sbo) {
                new (&m_storage.stack_storage) Type(std::forward<T>(t));
            } else {
                m_storage.heap_ptr = std::make_unique<Type>(std::forward<T>(t));
            }
            m_initialized = true;
        } else {
            if constexpr (ComparableType<Type>) {
                Type newValue(std::forward<T>(t));
                Type *current_ptr = use_sbo ? getStackPtr() : m_storage.heap_ptr.get();
                changed = *current_ptr != newValue;
                if (changed) {
                    *current_ptr = std::move(newValue);
                }
            } else {
                Type *current_ptr = use_sbo ? getStackPtr() : m_storage.heap_ptr.get();
                *current_ptr = std::forward<T>(t);
            }
        }
        
        return changed;
    }

    /**
     * @brief Get the raw pointer to the managed resource.
     * 
     * @return Type* Raw pointer to the resource.
     */
    [[nodiscard]] Type *getRawPtr() const {
        ConditionalSharedLock<ConditionalSharedMutex> lock(m_resourceMutex);
        if (!m_initialized) {
            REACTION_THROW_NULL_POINTER("resource pointer access");
        }
        
        if constexpr (use_sbo) {
            return getStackPtr();
        } else {
            return m_storage.heap_ptr.get();
        }
    }

    /**
     * @brief Check if Small Buffer Optimization is being used for this type.
     * 
     * @return true if using SBO (stack storage), false if using heap storage.
     */
    [[nodiscard]] static constexpr bool isUsingSBO() noexcept {
        return use_sbo;
    }

    /**
     * @brief Get the size of the storage being used.
     * 
     * @return Size in bytes.
     */
    [[nodiscard]] static constexpr size_t getStorageSize() noexcept {
        if constexpr (use_sbo) {
            return sizeof(Type);
        } else {
            return sizeof(std::unique_ptr<Type>);
        }
    }

protected:
    mutable ConditionalSharedMutex m_resourceMutex; ///< Conditional mutex for thread-safe resource access.

private:
    /**
     * @brief Get pointer to stack-stored object (for SBO case).
     */
    [[nodiscard]] Type *getStackPtr() const noexcept {
        return reinterpret_cast<Type *>(const_cast<std::byte *>(m_storage.stack_storage));
    }

    /**
     * @brief Union to store either stack or heap allocated data.
     */
    union Storage {
        alignas(Type) std::byte stack_storage[sizeof(Type)];  ///< Stack storage for small objects
        std::unique_ptr<Type> heap_ptr;                       ///< Heap pointer for large objects
        
        Storage() {}  // Default constructor does nothing
        ~Storage() {} // Destructor does nothing, managed by OptimizedResource
    } m_storage;

    bool m_initialized; ///< Flag to track if the resource is initialized
};

/**
 * @brief Specialization of OptimizedResource for Void type.
 * 
 * Since Void contains no data, getValue simply returns a default constructed Void.
 */
template <>
class OptimizedResource<Void> : public ObserverNode {
public:
    /**
     * @brief Return a default constructed Void.
     * 
     * @return Void An empty value.
     */
    Void getValue() const noexcept {
        return Void{};
    }

    /**
     * @brief Void specialization always uses "SBO" (no storage needed).
     */
    [[nodiscard]] static constexpr bool isUsingSBO() noexcept {
        return true;
    }

    /**
     * @brief Void has zero storage size.
     */
    [[nodiscard]] static constexpr size_t getStorageSize() noexcept {
        return 0;
    }
};

} // namespace reaction