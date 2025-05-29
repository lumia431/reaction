#pragma once

#include "reaction/concept.h"
#include "reaction/log.h"
#include <atomic>
#include <unordered_map>
#include <unordered_set>

namespace reaction {

enum class ReactionError {
    CycleDepErr = -2,
    ReturnTypeErr,
    NoErr
};

class UniqueID {
public:
    UniqueID() : m_id(generate()) {
    }

    operator uint64_t() {
        return m_id;
    }

    bool operator==(const UniqueID &other) const {
        return m_id == other.m_id;
    }

private:
    uint64_t m_id;

    static uint64_t generate() {
        static std::atomic<uint64_t> counter{0};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }
    friend struct std::hash<UniqueID>;
};

using NodePtr = std::shared_ptr<ObserverNode>;
using NodeWeak = std::weak_ptr<ObserverNode>;

} // namespace reaction

namespace std {
template <>
struct hash<reaction::UniqueID> {
    std::size_t operator()(const reaction::UniqueID &uid) const noexcept {
        return std::hash<uint64_t>{}(uid.m_id);
    }
};

struct WeakPtrHash {
    size_t operator()(const reaction::NodeWeak &wp) const {
        return std::hash<reaction::ObserverNode *>()(wp.lock().get());
    }
};

struct WeakPtrEqual {
    bool operator()(const reaction::NodeWeak &a, const reaction::NodeWeak &b) const {
        return a.lock() == b.lock();
    }
};

} // namespace std

namespace reaction {
using NodeSet = std::unordered_set<NodeWeak, std::WeakPtrHash, std::WeakPtrEqual>;
using NodeMap = std::unordered_map<NodeWeak, uint8_t, std::WeakPtrHash, std::WeakPtrEqual>;

using NodeSetRef = std::reference_wrapper<NodeSet>;
using NodeMapRef = std::reference_wrapper<NodeMap>;
} // namespace reaction
