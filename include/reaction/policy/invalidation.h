/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/core/concept.h"

namespace reaction {

/**
 * @brief Base template for invalidation strategies with CRTP optimization.
 *
 * @tparam Derived The derived invalidation strategy type
 */
template <typename Derived> struct InvalidationBase {
    template <typename Source>
    constexpr void handleInvalid(Source&& source) noexcept(
        noexcept(static_cast<Derived*>(this)->handleInvalidImpl(std::forward<Source>(source)))) {
        static_cast<Derived*>(this)->handleInvalidImpl(std::forward<Source>(source));
    }
};

/**
 * @brief Optimized close strategy with enhanced type safety.
 *
 * On invalidation, this strategy closes the reactive source,
 * cutting off its output from the dependency graph.
 */
struct CloseHandle : InvalidationBase<CloseHandle> {
    template <typename Source> constexpr void handleInvalidImpl(Source&& source) {
        if constexpr (IsReactSource<std::remove_cvref_t<Source>>) {
            source.close();
        }
    }
};

/**
 * @brief Optimized keep strategy.
 *
 * On invalidation, this strategy performs no action.
 * The value remains unchanged and stays in the graph.
 */
struct KeepHandle : InvalidationBase<KeepHandle> {
    template <typename Source>
    constexpr void handleInvalidImpl([[maybe_unused]] Source&& source) noexcept {
        // Do nothing - optimized away at compile time
    }
};

/**
 * @brief Enhanced last-value strategy with better exception safety.
 *
 * On invalidation, this strategy captures the last known value
 * and replaces the expression with a constant returning that value.
 */
struct LastHandle : InvalidationBase<LastHandle> {
    template <typename Source> constexpr void handleInvalidImpl(Source&& source) {
        if constexpr (IsReactSource<std::remove_cvref_t<Source>>) {
            try {
                auto val = source.get();
                source.set([val = std::move(val)]() noexcept { return val; });
            } catch (...) {
                // If getting the value fails, fall back to close strategy
                source.close();
            }
        }
    }
};

} // namespace reaction
