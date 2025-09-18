/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/observer.h"

namespace reaction {

/**
 * @brief A reactive resource wrapper managing a value of type Type.
 *
 * Inherits from ObserverNode, so it can participate in the reactive graph.
 * Wraps the resource in a std::unique_ptr to manage its lifetime.
 *
 * @tparam Type The type of the resource to manage.
 */
template <typename Type>
class Resource : public ObserverNode {
public:
    /**
     * @brief Default constructor initializes with nullptr.
     */
    Resource() : m_ptr(nullptr) {
    }

    /**
     * @brief Constructor that initializes the resource with a forwarded value.
     *
     * @tparam T Type of the initialization argument, constrained so it cannot be another Resource<Type>.
     * @param t The initialization value for the resource.
     */
    template <typename T>
        requires(!std::is_same_v<std::remove_cvref_t<T>, Resource<Type>>)
    Resource(T &&t) : m_ptr(std::make_unique<Type>(std::forward<T>(t))) {
    }

    // Delete copy constructor to avoid copying unique_ptr
    Resource(const Resource &) = delete;

    // Delete copy assignment operator to avoid copying unique_ptr
    Resource &operator=(const Resource &) = delete;

    /**
     * @brief Get a reference to the managed resource.
     *
     * Throws std::runtime_error if the resource is not initialized.
     *
     * @return Type& Reference to the managed resource.
     */
    [[nodiscard]] Type &getValue() const {
        if (!m_ptr) {
            throw std::runtime_error("Resource is not initialized");
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
        bool changed = true;
        if (!m_ptr) {
            m_ptr = std::make_unique<Type>(std::forward<T>(t));
        } else {
            if constexpr (ComparableType<Type>) {
                changed = *m_ptr != t;
            }
            *m_ptr = std::forward<T>(t);
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
        if (!this->m_ptr) {
            throw std::runtime_error("Attempt to get a null pointer");
        }
        return this->m_ptr.get();
    }

protected:
    std::unique_ptr<Type> m_ptr; ///< Unique pointer managing the resource.
};

/**
 * @brief An empty struct to represent void type in Resource specialization.
 */
struct Void {};

/**
 * @brief Specialization of Resource for Void type.
 *
 * Since Void contains no data, getValue simply returns a default constructed Void.
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
};

} // namespace reaction
