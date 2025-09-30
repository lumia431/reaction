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
#include "reaction/memory/sbo_resource.h"
#include <mutex>
#include <shared_mutex>

namespace reaction {

/**
 * @brief An empty struct to represent void type in Resource specialization.
 */
struct Void {};

// Forward declaration of ResourceImpl
template <typename Type>
class ResourceImpl;

/**
 * @brief A reactive resource wrapper managing a value of type Type.
 *
 * Inherits from ObserverNode, so it can participate in the reactive graph.
 * Automatically uses Small Buffer Optimization (SBO) when beneficial.
 *
 * @tparam Type The type of the resource to manage.
 */
template <typename Type>
class Resource : public std::conditional_t<memory::SBOTraits<Type>::is_sbo_eligible,
                                           memory::SBOResource<Type>,
                                           ResourceImpl<Type>>
{
private:
    using BaseType = std::conditional_t<memory::SBOTraits<Type>::is_sbo_eligible,
                                        memory::SBOResource<Type>,
                                        ResourceImpl<Type>>;

public:
    using BaseType::BaseType; // Inherit constructors

    // Expose whether this instance is using SBO
    [[nodiscard]] bool isUsingSBO() const noexcept {
        if constexpr (memory::SBOTraits<Type>::is_sbo_eligible) {
            return static_cast<const memory::SBOResource<Type> *>(this)->isUsingSBO();
        } else {
            return false;
        }
    }
};

/**
 * @brief Traditional heap-based resource implementation.
 *
 * This is the fallback implementation when SBO is not beneficial or disabled.
 */
template <typename Type>
class ResourceImpl : public ObserverNode {
public:
    /**
     * @brief Default constructor initializes with nullptr.
     */
    ResourceImpl() : m_ptr(nullptr) {
    }

    /**
     * @brief Constructor that initializes the resource with a forwarded value.
     *
     * @tparam T Type of the initialization argument, constrained so it cannot be another Resource<Type>.
     * @param t The initialization value for the resource.
     */
    template <typename T>
        requires(!std::is_same_v<std::remove_cvref_t<T>, ResourceImpl<Type>>)
    ResourceImpl(T &&t) : m_ptr(std::make_unique<Type>(std::forward<T>(t))) {
    }

    // Delete copy constructor to avoid copying unique_ptr
    ResourceImpl(const ResourceImpl &) = delete;

    // Delete copy assignment operator to avoid copying unique_ptr
    ResourceImpl &operator=(const ResourceImpl &) = delete;

    /**
     * @brief Get a copy of the managed resource value in a thread-safe manner.
     *
     * Throws std::runtime_error if the resource is not initialized.
     *
     * @return Type Copy of the managed resource.
     */
    [[nodiscard]] Type getValue() const {
        ConditionalSharedLock<ConditionalSharedMutex> lock(m_resourceMutex);
        if (!m_ptr) {
            REACTION_THROW_RESOURCE_NOT_INITIALIZED("Resource");
        }
        return *m_ptr;
    }

    /**
     * @brief Update the managed resource with a new value.
     *
     * If the resource is not yet initialized, create a new one with the forwarded value.
     * Otherwise, assign the forwarded value to the existing resource.
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
        if (!m_ptr) {
            m_ptr = std::make_unique<Type>(std::forward<T>(t));
        } else {
            if constexpr (ComparableType<Type>) {
                // Create a local copy to avoid data tearing during comparison
                Type newValue(std::forward<T>(t));
                changed = *m_ptr != newValue;
                if (changed) {
                    *m_ptr = std::move(newValue);
                }
            } else {
                *m_ptr = std::forward<T>(t);
            }
        }
        return changed;
    }

    /**
     * @brief Get the raw pointer to the managed resource.
     *
     * Throws std::runtime_error if the resource is not initialized.
     *
     * @return Type* Raw pointer to the resource.
     */
    [[nodiscard]] Type *getRawPtr() const {
        ConditionalSharedLock<ConditionalSharedMutex> lock(m_resourceMutex);
        if (!this->m_ptr) {
            REACTION_THROW_NULL_POINTER("resource pointer access");
        }
        return this->m_ptr.get();
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
                if (!m_ptr) {
                    REACTION_THROW_RESOURCE_NOT_INITIALIZED("ResourceImpl");
                }

                changed = operation(*m_ptr);

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

    /**
     * @brief Always returns false for heap-based ResourceImpl.
     */
    [[nodiscard]] bool isUsingSBO() const noexcept {
        return false;
    }

protected:
    mutable ConditionalSharedMutex m_resourceMutex; ///< Conditional mutex for thread-safe resource access.
    std::unique_ptr<Type> m_ptr;                    ///< Unique pointer managing the resource
};

/**
 * @brief Specialization of Resource for Void type.
 *
 * Since Void contains no data, getValue simply returns a default constructed Void.
 * Always uses SBO since no actual storage is needed.
 */
template <>
class Resource<Void> : public ObserverNode {
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
     * @brief Always returns true for Void (no storage needed).
     */
    [[nodiscard]] bool isUsingSBO() const noexcept {
        return true;
    }
};

} // namespace reaction

// Void specialization for SBOResource (placed here to avoid circular dependency)
namespace reaction::memory {

/**
 * @brief Specialization for Void type (maintains original behavior).
 */
template <>
class SBOResource<reaction::Void> : public ObserverNode {
public:
    /**
     * @brief Return default Void value.
     */
    reaction::Void getValue() const noexcept {
        return reaction::Void{};
    }

    /**
     * @brief Always reports using SBO (no actual storage needed).
     */
    [[nodiscard]] bool isUsingSBO() const noexcept {
        return true;
    }
};

} // namespace reaction::memory
