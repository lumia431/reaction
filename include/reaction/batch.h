/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/observer.h"
#include <set>

namespace reaction {

/**
 * @brief Comparison functor for ordering weak pointers to ObserverNodes by their depth.
 *
 * Used to maintain nodes in a multiset ordered by their depth value.
 * The comparison is performed on locked shared_ptr instances.
 */
struct BatchCompare {
    /**
     * @brief Compare two weak pointers to ObserverNodes by their depth.
     * @param lhs Left-hand side weak pointer
     * @param rhs Right-hand side weak pointer
     * @return true if lhs's depth is less than rhs's depth
     */
    bool operator()(const NodeWeak &lhs, const NodeWeak &rhs) const {
        return lhs.lock()->m_depth < rhs.lock()->m_depth;
    }
};

/**
 * @brief Represents a batch operation that tracks and manages observer nodes.
 *
 * The Batch class executes a function while collecting all observer nodes
 * that are accessed during execution. It maintains these nodes and can
 * trigger value change notifications when the batch is executed.
 */
class Batch {
public:
    /**
     * @brief Construct a new Batch with the given function.
     *
     * @tparam F Type of the callable function
     * @param f Function to execute as part of this batch
     *
     * During construction:
     * 1. Sets up a BatchFunGuard to collect accessed observer nodes
     * 2. Executes the function
     * 3. Collects all observer nodes that were accessed
     */
    template <InvocableType F>
    Batch(F &&f) : m_fun(std::forward<F>(f)) {
        BatchFunGuard g{[this](const NodePtr &node) {
            ObserverGraph::getInstance().collectObservers(node, m_observers);
        }};
        std::invoke(f);
        for (auto &node : m_observers) {
            m_batchNodes.insert(node);
        }
    }

    // Disable copying and moving
    Batch(const Batch &) = delete;
    Batch(Batch &&) = delete;

    /**
     * @brief Execute the batch operation.
     *
     * 1. Invokes the stored function
     * 2. Triggers valueChanged() on all collected observer nodes
     */
    void execute() {
        BatchExeGuard g{true};
        std::invoke(m_fun);
        for (auto &node : m_batchNodes) {
            if (auto wp = node.lock()) [[likely]] wp->changedNoNotify();
        }
    }

private:
    NodeSet m_observers;                                ///< Collection of observer nodes accessed during batch
    std::multiset<NodeWeak, BatchCompare> m_batchNodes; ///< Nodes tracked by this batch, ordered by depth
    std::function<void()> m_fun;                        ///< The function to execute for this batch
};

} // namespace reaction