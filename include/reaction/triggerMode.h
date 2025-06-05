#pragma once

#include <functional>

namespace reaction {

struct AlwaysTrig {
    bool checkTrigger() {
        return true;
    }
};

struct ChangeTrig {
    bool checkTrigger() {
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

    bool checkTrigger() {
        return std::invoke(m_filterFun);
    }

private:
    template <typename F, typename... A>
    auto createFun(F &&f, A &&...args) {
        return [f = std::forward<F>(f), ... args = args.getPtr()]() {
            return std::invoke(f, args->get()...);
        };
    }

    std::function<bool()> m_filterFun = []() { return true; };
};

} // namespace reaction