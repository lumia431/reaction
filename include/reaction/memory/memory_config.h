/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <cstddef>
#include <type_traits>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace reaction::memory {

/**
 * @brief Small Buffer Optimization (SBO) configuration constants.
 */
struct SBOConfig {
    // Size thresholds for SBO decision making
    static constexpr size_t ALWAYS_SBO_SIZE = 16;    ///< Always use SBO for types ≤ 16 bytes
    static constexpr size_t CONDITIONAL_SBO_SIZE = 32; ///< Conditional SBO for 17-32 bytes
    static constexpr size_t MAX_SBO_SIZE = 64;        ///< Maximum size for SBO consideration

    // Stack usage limits per thread
    static constexpr size_t MAX_SBO_STACK_USAGE = 64 * 1024; ///< 64KB max SBO stack usage per thread
    static constexpr size_t STACK_USAGE_WARNING = 32 * 1024; ///< Warning threshold for stack usage

    // Alignment requirements
    static constexpr size_t MAX_SBO_ALIGNMENT = alignof(std::max_align_t);
};

/**
 * @brief Type traits for SBO eligibility checking.
 */
template<typename T>
struct SBOTraits {
    static constexpr bool is_small_enough = sizeof(T) <= SBOConfig::MAX_SBO_SIZE;
    static constexpr bool is_aligned_properly = alignof(T) <= SBOConfig::MAX_SBO_ALIGNMENT;
    static constexpr bool is_nothrow_movable = std::is_nothrow_move_constructible_v<T>;
    static constexpr bool is_nothrow_destructible = std::is_nothrow_destructible_v<T>;

    // Basic SBO eligibility (compile-time check)
    static constexpr bool is_sbo_eligible = is_small_enough &&
                                           is_aligned_properly &&
                                           is_nothrow_movable &&
                                           is_nothrow_destructible;

    // Always use SBO for very small types
    static constexpr bool always_sbo = sizeof(T) <= SBOConfig::ALWAYS_SBO_SIZE && is_sbo_eligible;

    // Conditional SBO for medium-sized types
    static constexpr bool conditional_sbo = sizeof(T) <= SBOConfig::CONDITIONAL_SBO_SIZE &&
                                           sizeof(T) > SBOConfig::ALWAYS_SBO_SIZE &&
                                           is_sbo_eligible;

    // Force heap allocation for large or problematic types
    static constexpr bool force_heap = !is_sbo_eligible || sizeof(T) > SBOConfig::MAX_SBO_SIZE;
};

/**
 * @brief Specializations for common types to override default behavior.
 */
template<>
struct SBOTraits<std::string> {
    static constexpr bool is_sbo_eligible = false;
    static constexpr bool always_sbo = false;
    static constexpr bool conditional_sbo = false;
    static constexpr bool force_heap = true;
};

template<typename T, typename Alloc>
struct SBOTraits<std::vector<T, Alloc>> {
    static constexpr bool is_sbo_eligible = false;
    static constexpr bool always_sbo = false;
    static constexpr bool conditional_sbo = false;
    static constexpr bool force_heap = true;
};

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
struct SBOTraits<std::unordered_map<Key, Value, Hash, Equal, Alloc>> {
    static constexpr bool is_sbo_eligible = false;
    static constexpr bool always_sbo = false;
    static constexpr bool conditional_sbo = false;
    static constexpr bool force_heap = true;
};

} // namespace reaction::memory