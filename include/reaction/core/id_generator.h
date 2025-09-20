/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <atomic>
#include <cstdint>

namespace reaction {

/**
 * @brief Class representing a unique identifier.
 *
 * Generates a unique 64-bit integer ID for each instance.
 * Provides implicit conversion to uint64_t and equality comparison.
 * Thread-safe ID generation using atomic operations.
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
    constexpr operator uint64_t() const noexcept {
        return m_id;
    }

    /**
     * @brief Equality comparison operator.
     * @param other Another UniqueID instance.
     * @return True if IDs are equal, false otherwise.
     */
    constexpr bool operator==(const UniqueID &other) const noexcept {
        return m_id == other.m_id;
    }

    /**
     * @brief Inequality comparison operator.
     * @param other Another UniqueID instance.
     * @return True if IDs are not equal, false otherwise.
     */
    constexpr bool operator!=(const UniqueID &other) const noexcept {
        return m_id != other.m_id;
    }

    /**
     * @brief Less-than comparison operator for ordering.
     * @param other Another UniqueID instance.
     * @return True if this ID is less than other ID.
     */
    constexpr bool operator<(const UniqueID &other) const noexcept {
        return m_id < other.m_id;
    }

    /**
     * @brief Get the raw ID value.
     * @return The unique ID as uint64_t.
     */
    constexpr uint64_t value() const noexcept {
        return m_id;
    }

private:
    uint64_t m_id; ///< Internal unique identifier.

    /**
     * @brief Static function to generate a unique 64-bit ID.
     * Uses atomic counter for thread-safe ID generation.
     * @return A unique 64-bit unsigned integer.
     */
    static uint64_t generate() noexcept {
        static std::atomic<uint64_t> counter{0};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }

    // Grant std::hash access to private members.
    friend struct std::hash<UniqueID>;
};

} // namespace reaction

// === STL Hash Support ===

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

} // namespace std
