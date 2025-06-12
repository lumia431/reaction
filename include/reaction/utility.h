/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/concept.h"
#include "reaction/log.h"
#include <atomic>
#include <exception>
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
 * @brief Alias for an unordered map from weak pointer to uint8_t,
 * representing counts or flags, with custom hash and equality.
 */
using NodeMap = std::unordered_map<NodeWeak, uint8_t, std::WeakPtrHash, std::WeakPtrEqual>;

/**
 * @brief Reference wrapper type for NodeSet.
 */
using NodeSetRef = std::reference_wrapper<NodeSet>;

/**
 * @brief Reference wrapper type for NodeMap.
 */
using NodeMapRef = std::reference_wrapper<NodeMap>;

} // namespace reaction
