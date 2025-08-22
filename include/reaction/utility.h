/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/concept.h"
#include <atomic>
#include <functional>
#include <stdexcept>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace reaction {

/**
 * @brief Class representing a unique identifier.
 *
 * Generates a unique 64-bit integer ID for each instance.
 * Provides implicit conversion to uint64_t and equality comparison.
 */
class UniqueID {
public:
    /**
     * @brief Constructs a UniqueID by generating a unique value.
     */
    UniqueID() : m_id(generate()) {
    }

    /**
     * @brief Implicit conversion operator to uint64_t.
     * @return The unique ID as uint64_t.
     */
    operator uint64_t() {
        return m_id;
    }

    /**
     * @brief Equality comparison operator.
     * @param other Another UniqueID instance.
     * @return True if IDs are equal, false otherwise.
     */
    bool operator==(const UniqueID &other) const {
        return m_id == other.m_id;
    }

private:
    uint64_t m_id; ///< Internal unique identifier.

    /**
     * @brief Static function to generate a unique 64-bit ID.
     * Uses atomic counter for thread-safe ID generation.
     * @return A unique 64-bit unsigned integer.
     */
    static uint64_t generate() {
        static std::atomic<uint64_t> counter{0};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }

    // Grant std::hash access to private members.
    friend struct std::hash<UniqueID>;
};

// Alias for shared and weak pointers to ObserverNode.
using NodePtr = std::shared_ptr<ObserverNode>;
using NodeWeak = std::weak_ptr<ObserverNode>;

/**
 * @brief RAII guard for managing global state variables.
 * 
 * This template provides automatic cleanup of global variables when the guard
 * goes out of scope. It's used to temporarily set global state and ensure
 * it's restored to a default value when done.
 * 
 * @tparam val Reference to the global variable to manage
 * @tparam guard Default value to restore when destructor is called
 */
template <auto &val, auto guard>
struct GlobalGuard {
    /**
     * @brief Constructor that sets the global variable to the provided value.
     * 
     * @tparam T Type of the value to set (deduced)
     * @param t Value to assign to the global variable
     */
    template <typename T>
        requires(!std::is_same_v<std::remove_cvref_t<T>, GlobalGuard>)
    explicit GlobalGuard(T &&t) {
        val = std::forward<T>(t);
    }

    /**
     * @brief Destructor that restores the global variable to its guard value.
     */
    ~GlobalGuard() {
        val = guard;
    }
};

// === Global State Variables for Reactive System ===

/**
 * @brief Global function pointer for dependency registration.
 * 
 * When set, this function is called whenever a reactive value is accessed,
 * allowing automatic dependency tracking during expression evaluation.
 */
inline std::function<void(const NodePtr &)> g_reg_fun = nullptr;

/**
 * @brief Global function pointer for batch operation tracking.
 * 
 * When set, this function is called when reactive values are modified
 * during batch operations, allowing deferred notification handling.
 */
inline std::function<void(const NodePtr &)> g_batch_fun = nullptr;

/**
 * @brief Global flag indicating whether a batch operation is currently executing.
 * 
 * When true, reactive notifications are deferred until the batch completes.
 */
inline bool g_batch_execute = false;

// === Type Aliases for RAII Guards ===

using RegFunGuard = GlobalGuard<g_reg_fun, nullptr>;     ///< RAII guard for dependency registration
using BatchFunGuard = GlobalGuard<g_batch_fun, nullptr>; ///< RAII guard for batch function tracking  
using BatchExeGuard = GlobalGuard<g_batch_execute, false>; ///< RAII guard for batch execution flag

} // namespace reaction

namespace std {

/**
 * @brief Specialization of std::hash for reaction::UniqueID.
 * Hashes the internal 64-bit ID.
 */
template <>
struct hash<reaction::UniqueID> {
    size_t operator()(const reaction::UniqueID &uid) const noexcept {
        return std::hash<uint64_t>{}(uid.m_id);
    }
};

/**
 * @brief Hash function object for weak_ptr to ObserverNode.
 *
 * Locks the weak_ptr to get raw pointer and hashes it.
 */
struct WeakPtrHash {
    size_t operator()(const reaction::NodeWeak &wp) const {
        return std::hash<reaction::ObserverNode *>()(wp.lock().get());
    }
};

/**
 * @brief Equality comparison for weak_ptr to ObserverNode.
 *
 * Two weak_ptrs are equal if their locked shared_ptrs are equal.
 */
struct WeakPtrEqual {
    bool operator()(const reaction::NodeWeak &a, const reaction::NodeWeak &b) const {
        return a.lock() == b.lock();
    }
};

} // namespace std

namespace reaction {

/**
 * @brief Alias for an unordered set of weak pointers to ObserverNode,
 * using custom hash and equality functions.
 */
using NodeSet = std::unordered_set<NodeWeak, std::WeakPtrHash, std::WeakPtrEqual>;

/**
 * @brief Reference wrapper type for NodeSet.
 */
using NodeSetRef = std::reference_wrapper<NodeSet>;

} // namespace reaction
