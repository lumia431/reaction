/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <functional>
#include <memory>
#include <unordered_set>

namespace reaction {

// Forward declarations
class ObserverNode;

// === Core Type Aliases ===

/**
 * @brief Shared pointer to ObserverNode.
 *
 * Used for managing reactive node lifetimes with automatic memory management.
 */
using NodePtr = std::shared_ptr<ObserverNode>;

/**
 * @brief Weak pointer to ObserverNode.
 *
 * Used for non-owning references to reactive nodes to avoid circular dependencies.
 */
using NodeWeak = std::weak_ptr<ObserverNode>;

} // namespace reaction

// === STL Extensions for Node Pointers ===

namespace std {

/**
 * @brief Hash function object for weak_ptr to ObserverNode.
 *
 * Locks the weak_ptr to get raw pointer and hashes it.
 * Used for storing NodeWeak in unordered containers.
 */
struct WeakPtrHash {
    size_t operator()(const reaction::NodeWeak& wp) const {
        return std::hash<reaction::ObserverNode*>()(wp.lock().get());
    }
};

/**
 * @brief Equality comparison for weak_ptr to ObserverNode.
 *
 * Two weak_ptrs are equal if their locked shared_ptrs are equal.
 * Used for storing NodeWeak in unordered containers.
 */
struct WeakPtrEqual {
    bool operator()(const reaction::NodeWeak& a, const reaction::NodeWeak& b) const {
        return a.lock() == b.lock();
    }
};

} // namespace std

namespace reaction {

// === Container Type Aliases ===

/**
 * @brief Unordered set of weak pointers to ObserverNode.
 *
 * Uses custom hash and equality functions for weak_ptr support.
 * Commonly used for storing collections of reactive node references.
 */
using NodeSet = std::unordered_set<NodeWeak, std::WeakPtrHash, std::WeakPtrEqual>;

/**
 * @brief Reference wrapper type for NodeSet.
 *
 * Used when we need to store references to NodeSet objects.
 */
using NodeSetRef = std::reference_wrapper<NodeSet>;

} // namespace reaction
