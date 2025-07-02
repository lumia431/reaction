/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/observerNode.h"
#include <functional>
#include <set>

namespace reaction {

/**
 * @brief Thread-local batch processing function storage.
 *
 * Stores the current batch processing function that will be called
 * when observer nodes are accessed during batch operations.
 */
inline thread_local std::function<void(NodePtr)> g_batch_fun = nullptr;

/**
 * @brief RAII guard for managing the current batch processing function.
 *
 * Sets the thread-local batch function on construction
 * and clears it on destruction.
 */
struct BatchGuard {
    /**
     * @brief Construct a new BatchGuard with the given function.
     * @param f The batch processing function to set as current
     */
    explicit BatchGuard(const std::function<void(NodePtr)> &f) {
        g_batch_fun = f;
    }

    /**
     * @brief Destroy the BatchGuard and clear the current batch function.
     */
    ~BatchGuard() {
        g_batch_fun = nullptr;
    }
};

/**
 * @brief Comparison functor for ordering weak pointers to ObserverNodes by their level.
 *
 * Used to maintain nodes in a multiset ordered by their level value.
 * The comparison is performed on locked shared_ptr instances.
 */
struct BatchCompare {
    /**
     * @brief Compare two weak pointers to ObserverNodes by their level.
     * @param lhs Left-hand side weak pointer
     * @param rhs Right-hand side weak pointer
     * @return true if lhs's level is less than rhs's level
     */
    bool operator()(const NodeWeak &lhs, const NodeWeak &rhs) const {
        return lhs.lock()->level < rhs.lock()->level;
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
     * 1. Sets up a BatchGuard to collect accessed observer nodes
     * 2. Executes the function
     * 3. Collects all observer nodes that were accessed
     */
    template <typename F>
    Batch(F &&f) : m_fun(std::forward<F>(f)) {
        BatchGuard g{[this](NodePtr node) {
            node->batchCount++;
            ObserverGraph::getInstance().collectObservers(node, m_observers);
        }};
        std::invoke(f);
        for (auto &node : m_observers) {
            m_batchNodes.insert(node);
        }
    }

    /**
     * @brief Destroy the Batch object.
     *
     * Decrements the batchCount for all tracked observer nodes.
     */
    ~Batch() {
        for (auto &node : m_batchNodes) {
            if (auto wp = node.lock()) wp->batchCount--;
        }
    }

    // Disable copying and moving
    Batch(const Batch &) = delete;
    Batch(const Batch &&) = delete;

    /**
     * @brief Execute the batch operation.
     *
     * 1. Invokes the stored function
     * 2. Triggers valueChanged() on all collected observer nodes
     */
    void execute() {
        std::invoke(m_fun);
        for (auto &node : m_batchNodes) {
            if (auto wp = node.lock()) wp->valueChanged();
        }
    }

private:
    NodeSet m_observers;                                ///< Collection of observer nodes accessed during batch
    std::multiset<NodeWeak, BatchCompare> m_batchNodes; ///< Nodes tracked by this batch, ordered by level
    std::function<void()> m_fun;                        ///< The function to execute for this batch
};

} // namespace reaction