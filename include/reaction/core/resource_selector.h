/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/core/resource.h"
#include "reaction/core/resource_optimized.h"

namespace reaction {

/**
 * @brief Configuration macro to enable Small Buffer Optimization.
 * 
 * Define REACTION_ENABLE_SBO before including headers to enable SBO.
 * By default, SBO is disabled to maintain backward compatibility.
 */
#ifndef REACTION_ENABLE_SBO
#define REACTION_ENABLE_SBO 0
#endif

/**
 * @brief Type alias that selects the appropriate Resource implementation.
 * 
 * When REACTION_ENABLE_SBO is defined, uses OptimizedResource with SBO.
 * Otherwise, uses the standard Resource implementation.
 * 
 * @tparam Type The type of the resource to manage.
 */
template <typename Type>
#if REACTION_ENABLE_SBO
using ResourceImpl = OptimizedResource<Type>;
#else
using ResourceImpl = Resource<Type>;
#endif

/**
 * @brief Helper struct to provide information about the selected resource implementation.
 */
struct ResourceInfo {
    /**
     * @brief Check if Small Buffer Optimization is enabled.
     * 
     * @return true if SBO is enabled, false otherwise.
     */
    [[nodiscard]] static constexpr bool isSBOEnabled() noexcept {
        return REACTION_ENABLE_SBO;
    }

    /**
     * @brief Get information about a specific type's storage strategy.
     * 
     * @tparam Type The type to check.
     * @return String describing the storage strategy.
     */
    template <typename Type>
    [[nodiscard]] static constexpr const char* getStorageStrategy() noexcept {
        if constexpr (!REACTION_ENABLE_SBO) {
            return "heap";
        } else {
            if constexpr (should_use_sbo_v<Type>) {
                return "stack (SBO)";
            } else {
                return "heap";
            }
        }
    }

    /**
     * @brief Get the storage size for a specific type.
     * 
     * @tparam Type The type to check.
     * @return Size in bytes.
     */
    template <typename Type>
    [[nodiscard]] static constexpr size_t getStorageSize() noexcept {
        return ResourceImpl<Type>::getStorageSize();
    }
};

} // namespace reaction