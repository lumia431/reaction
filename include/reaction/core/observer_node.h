/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/concurrency/thread_safety.h"
#include "reaction/core/types.h"
#include <memory>

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
    mutable ConditionalSharedMutex m_observersMutex; ///< Mutex for thread-safe observer access
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
    void updateDepth(uint8_t depth) noexcept {
        m_depth = std::max(depth, m_depth);
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
        // Create a snapshot of observers to avoid holding lock during notifications
        NodeSet snapshot;
        {
            ConditionalSharedLock<ConditionalSharedMutex> lock(m_observersMutex);
            snapshot = m_observers;
        }
        for (auto &observer : snapshot) {
            if (auto wp = observer.lock()) [[likely]]
                wp->valueChanged(changed);
        }
    }

private:
    uint8_t m_depth = 0; ///< Depth of the node in reactive chain.
    NodeSet m_observers; ///< Direct observers of this node.
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
