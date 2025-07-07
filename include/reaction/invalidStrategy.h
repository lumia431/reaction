/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/concept.h"

namespace reaction {

/**
 * @brief Close strategy.
 *
 * On invalidation, this strategy closes the reactive source,
 * cutting off its output from the dependency graph.
 */
struct CloseStra {
    template <typename Source>
    void handleInvalid(Source &&source) {
        if constexpr (IsReactSource<std::remove_cvref_t<Source>>) {
            source.close();
        }
    }
};

/**
 * @brief Keep strategy.
 *
 * On invalidation, this strategy performs no action.
 * The value remains unchanged and stays in the graph.
 */
struct KeepStra {
    template <typename Source>
    void handleInvalid([[maybe_unused]] Source &&source) {
        // Do nothing
    }
};

/**
 * @brief Last-value strategy.
 *
 * On invalidation, this strategy captures the last known value
 * and replaces the expression with a constant returning that value.
 */
struct LastStra {
    template <typename Source>
    void handleInvalid(Source &&source) {
        if constexpr (IsReactSource<std::remove_cvref_t<Source>>) {
            auto val = source.get();
            source.set([=]() { return val; });
        }
    }
};

} // namespace reaction
