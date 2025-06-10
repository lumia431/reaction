/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/concept.h"

namespace reaction {

struct CloseStra {
    template <typename Source>
    void handleInvalid(Source &&source) {
        if constexpr (IsReactSource<std::decay_t<Source>>) {
            source.close();
        }
    }
};

struct KeepStra {
    template <typename Source>
    void handleInvalid([[maybe_unused]] Source &&source) {
    }
};

struct LastStra {
    template <typename Source>
    void handleInvalid(Source &&source) {
        if constexpr (IsReactSource<std::decay_t<Source>>) {
            auto val = source.get();
            source.set([=]() { return val; });
        }
    }
};

} // namespace reaction