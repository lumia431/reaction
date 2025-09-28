/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/concurrency/thread_manager.h"
#include "reaction/core/types.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace reaction {

// Forward declarations
class ObserverGraph;

/**
 * @brief Reactive graph node base class.
 *
 * All reactive nodes should inherit from this base.
 * It manages notification propagation and observer lists.
 */
class ObserverNode : public std::enable_shared_from_this<ObserverNode> {
public:
    virtual ~ObserverNode() = default;

    /**
     * @brief Trigger downstream notifications.
     *
     * By default, triggers notify().
     * @param changed Whether the node's value has changed.
     */
    virtual void valueChanged(bool changed = true) {
        this->notify(changed);
    }

    /**
     * @brief Handle value change without triggering downstream notifications.
     *
     * This method is called when a node's value changes but we don't want
     * to immediately propagate the change to observers. This is useful in
     * batch operations where notifications are deferred until the end.
     *
     * @param changed Whether the node's value has actually changed (default: true)
     */
    virtual void changedNoNotify([[maybe_unused]] bool changed = true) {}

    /**
     * @brief Update the depth of this node in the reactive dependency graph.
     *
     * The depth represents how many levels deep this node is in the dependency chain.
     * This is used for ordering nodes during batch operations and cycle detection.
     * The depth is set to the maximum of the current depth and the provided depth.
     *
     * @param depth The new depth value to consider
     */
    void updateDepth(uint16_t depth) noexcept {
        REACTION_REGISTER_THREAD();
        if (ThreadManager::getInstance().isThreadSafetyEnabled()) {
            // Use atomic compare-and-swap for thread-safe depth updates
            uint16_t currentDepth = m_depth.load(std::memory_order_relaxed);
            uint16_t newDepth = std::max(depth, currentDepth);
            while (newDepth > currentDepth && 
                   !m_depth.compare_exchange_weak(currentDepth, newDepth, std::memory_order_relaxed)) {
                newDepth = std::max(depth, currentDepth);
            }
        } else {
            // Single-threaded: direct assignment
            m_depth.store(std::max(depth, m_depth.load(std::memory_order_relaxed)), std::memory_order_relaxed);
        }
    }

    /**
     * @brief Update all observer dependencies at once.
     *
     * Resets current observers, then adds given nodes as new observers.
     * @tparam Args Parameter pack for node pointers.
     * @param args Nodes to observe.
     */
    template <typename... Args>
    void updateObservers(Args &&...args);

    /**
     * @brief Add a single observer node.
     * @param node Observer node to add.
     */
    void addOneObserver(const NodePtr &node);

    /**
     * @brief Notify observers and delayed repeat nodes.
     * @param changed Whether the node's value has changed.
     */
    void notify(bool changed = true) {
        if (ThreadManager::getInstance().isThreadSafetyEnabled()) {
            // Copy observers under lock to avoid holding lock during callbacks
            std::vector<std::shared_ptr<ObserverNode>> observersCopy;
            {
                ConditionalSharedLock<ConditionalSharedMutex> lock(m_observersMutex);
                observersCopy.reserve(m_observers.size());
                for (auto &observer : m_observers) {
                    if (auto wp = observer.lock()) [[likely]] {
                        observersCopy.push_back(wp);
                    }
                }
            }

            // Notify observers outside of lock to avoid deadlocks
            for (auto &observer : observersCopy) {
                observer->valueChanged(changed);
            }
        } else {
            // Single-threaded mode: no need to copy, direct iteration
            for (auto &observer : m_observers) {
                if (auto wp = observer.lock()) [[likely]]
                    wp->valueChanged(changed);
            }
        }
    }

private:
    std::atomic<uint16_t> m_depth{0};               ///< Depth of the node in reactive chain.
    mutable ConditionalSharedMutex m_observersMutex; ///< Conditional mutex for thread-safe observers access.
    NodeSet m_observers;                             ///< Direct observers of this node.
    friend class ObserverGraph;
    friend struct BatchCompare;
};

} // namespace reaction

// Implementation of methods that require ObserverGraph
#include "reaction/graph/observer_graph.h"

namespace reaction {

template <typename... Args>
inline void ObserverNode::updateObservers(Args &&...args) {
    auto shared_this = shared_from_this();
    ObserverGraph::getInstance().updateObserversTransactional(shared_this, args...);
}

inline void ObserverNode::addOneObserver(const NodePtr &node) {
    ObserverGraph::getInstance().addObserver(shared_from_this(), node);
}

} // namespace reaction
