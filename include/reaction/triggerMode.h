/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <functional>

namespace reaction {

struct AlwaysTrig {
    bool checkTrig() {
        return true;
    }
};

struct ChangeTrig {
    bool checkTrig() {
        return m_changed;
    }

    void setChanged(bool changed) {
        m_changed = changed;
    }

private:
    bool m_changed = true;
};

struct FilterTrig {
    template <typename F, typename... A>
    void filter(F &&f, A &&...args) {
        m_filterFun = createFun(std::forward<F>(f), std::forward<A>(args)...);
    }

    bool checkTrig() {
        return std::invoke(m_filterFun);
    }

private:
    template <typename F, typename... A>
    auto createFun(F &&f, A &&...args) {
        return [f = std::forward<F>(f), ... args = args.getWeak()]() {
            return std::invoke(f, args.lock()->get()...);
        };
    }

    std::function<bool()> m_filterFun = []() { return true; };
};

} // namespace reaction