/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/core/observer_node.h"
#include "reaction/graph/observer_graph.h"
#include "reaction/concurrency/thread_safety.h"
#include "reaction/core/types.h"
#include "reaction/concurrency/global_state.h"
#include "reaction/core/raii_guards.h"
#include <iostream>
#include <set>
#include <atomic>

namespace reaction {

/**
 * @brief Enhanced comparison functor for ordering weak pointers to ObserverNodes by their depth.
 *
 * Provides safe comparison with null pointer checks and compile-time optimization.
 */
struct BatchCompare {
    /**
     * @brief Compare two weak pointers to ObserverNodes by their depth.
     * @param lhs Left-hand side weak pointer
     * @param rhs Right-hand side weak pointer
     * @return true if lhs's depth is less than rhs's depth
     */
    bool operator()(const NodeWeak& lhs, const NodeWeak& rhs) const noexcept {
        auto left = lhs.lock();
        auto right = rhs.lock();

        // Handle null pointers safely
        if (!left && !right) return false;
        if (!left) return true;  // null comes first
        if (!right) return false;

        return left->m_depth < right->m_depth;
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
     * 4. Registers this batch as active to prevent reset operations
     */
    template <InvocableType F>
    Batch(F &&f) : m_fun(std::forward<F>(f)), m_batchId(this) {
        REACTION_REGISTER_THREAD();
        auto g = makeBatchFunGuard([this](const NodePtr &node) {
            ConditionalUniqueLock<ConditionalSharedMutex> lock(m_batchMutex);
            ObserverGraph::getInstance().collectObservers(node, m_observers);
        });
        std::invoke(f);
        {
            ConditionalSharedLock<ConditionalSharedMutex> lock(m_batchMutex);
            for (auto &node : m_observers) {
                m_batchNodes.insert(node);
            }
        }

        // Register this batch as active to protect nodes from reset operations
        ObserverGraph::getInstance().registerActiveBatch(m_batchId, m_observers);
    }

    /**
     * @brief Destroy the Batch object.
     *
     * Unregisters this batch from active tracking if not already closed,
     * allowing reset operations on previously protected nodes.
     */
    ~Batch() {
        if (!m_isClosed.exchange(true)) {
            // Only unregister if we haven't already closed
            ObserverGraph::getInstance().unregisterActiveBatch(m_batchId);
        }
    }

    // Disable copying and moving
    Batch(const Batch &) = delete;
    Batch(Batch &&) = delete;

    /**
     * @brief Manually close the batch operation.
     *
     * This unregisters the batch from active tracking, allowing reset operations
     * on previously protected nodes. After calling close(), the batch is no longer
     * active and will not prevent reset operations.
     *
     * Calling close() multiple times is safe and has no effect after the first call.
     */
    void close() {
        if (!m_isClosed.exchange(true)) {
            // Only unregister if we haven't already closed
            ObserverGraph::getInstance().unregisterActiveBatch(m_batchId);
        }
    }

    /**
     * @brief Check if the batch has been closed.
     *
     * @return true if the batch has been closed, false otherwise
     */
    [[nodiscard]] bool isClosed() const noexcept {
        return m_isClosed.load();
    }

    /**
     * @brief Execute the batch operation.
     *
     * 1. Invokes the stored function
     * 2. Triggers valueChanged() on all collected observer nodes
     */
    void execute() {
        REACTION_REGISTER_THREAD();
        auto g = makeBatchExeGuard(true);
        std::invoke(m_fun);

        ConditionalSharedLock<ConditionalSharedMutex> lock(m_batchMutex);
        for (auto &node : m_batchNodes) {
            if (auto wp = node.lock()) [[likely]] wp->changedNoNotify();
        }
    }

private:
    NodeSet m_observers;                                ///< Collection of observer nodes accessed during batch
    std::multiset<NodeWeak, BatchCompare> m_batchNodes; ///< Nodes tracked by this batch, ordered by depth
    std::function<void()> m_fun;                        ///< The function to execute for this batch
    const void* m_batchId;                              ///< Unique identifier for this batch instance
    std::atomic<bool> m_isClosed{false};                ///< Whether the batch has been manually closed
    mutable ConditionalSharedMutex m_batchMutex;        ///< Mutex for thread-safe batch operations
};

} // namespace reaction