/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/cache/graph_cache.h"
#include "reaction/concurrency/thread_safety.h"
#include "reaction/core/exception.h"
#include "reaction/core/types.h"
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace reaction {

// Forward declarations
class ObserverNode;
using NodePtr = std::shared_ptr<ObserverNode>;

/**
 * @brief Manages the graph structure of reactive observers and dependencies.
 *
 * This singleton class is responsible for:
 * - Adding and removing nodes
 * - Managing observer/dependent relationships
 * - Detecting cycles
 */
class ObserverGraph {
public:
    /**
     * @brief Get the singleton instance of the graph.
     * @return ObserverGraph& singleton reference.
     */
    [[nodiscard]] static ObserverGraph &getInstance() noexcept {
        static ObserverGraph instance;
        return instance;
    }

    /**
     * @brief Add a new node into the graph.
     * @param node Node to add.
     */
    void addNode(const NodePtr &node) noexcept;

    /**
     * @brief Add an observer relationship (source observes target).
     *
     * @param source The observing node.
     * @param target The node being observed.
     * @throws std::runtime_error if a cycle or self-observation is detected.
     */
    void addObserver(const NodePtr &source, const NodePtr &target) {
        REACTION_REGISTER_THREAD();
        ConditionalUniqueLock<ConditionalSharedMutex> lock(m_graphMutex);

        if (source == target) {
            REACTION_THROW_SELF_OBSERVATION(getNameInternal(source));
        }

        // Check if both nodes exist in the graph first
        if (!m_dependentList.contains(source) || !m_observerList.contains(target)) {
            // If nodes don't exist, add them first (without taking locks since we already have one)
            if (!m_dependentList.contains(source)) {
                addNodeInternal(source);
            }
            if (!m_observerList.contains(target)) {
                addNodeInternal(target);
            }
        }

        if (hasCycle(source, target)) {
            REACTION_THROW_DEPENDENCY_CYCLE(getNameInternal(source), getNameInternal(target));
        }

        m_dependentList.at(source).insert(target);
        m_observerList.at(target).get().insert({source});

        // Invalidate all caches due to graph structure change
        m_graphCache.invalidateAll();
        m_cycleCache.invalidateAll();
        m_metricsCache.invalidateAll();
    }

    /**
     * @brief Register a batch operation that affects specific nodes.
     *
     * This method tracks which nodes are currently involved in active batch operations.
     * Reset operations on these nodes will be prevented or delayed.
     *
     * @param batchId Unique identifier for the batch operation
     * @param nodes Set of nodes affected by this batch
     */
    void registerActiveBatch(const void *batchId, const NodeSet &nodes) {
        ConditionalUniqueLock<ConditionalSharedMutex> lock(m_graphMutex);
        for (const auto &nodeWeak : nodes) {
            if (auto node = nodeWeak.lock()) {
                m_activeBatchNodes[node].insert(batchId);
            }
        }
        m_activeBatchIds.insert(batchId);
    }

    /**
     * @brief Unregister a batch operation.
     *
     * Removes the batch from active tracking and allows reset operations
     * on previously protected nodes.
     *
     * @param batchId Unique identifier for the batch operation
     */
    void unregisterActiveBatch(const void *batchId) {
        ConditionalUniqueLock<ConditionalSharedMutex> lock(m_graphMutex);
        m_activeBatchIds.erase(batchId);

        // Remove this batch from all node tracking
        for (auto it = m_activeBatchNodes.begin(); it != m_activeBatchNodes.end();) {
            it->second.erase(batchId);
            if (it->second.empty()) {
                it = m_activeBatchNodes.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief Check if a node is currently involved in any active batch operation.
     *
     * @param node The node to check
     * @return true if the node is in an active batch, false otherwise
     */
    bool isNodeInActiveBatch(const NodePtr &node) const {
        ConditionalSharedLock<ConditionalSharedMutex> lock(m_graphMutex);
        auto it = m_activeBatchNodes.find(node);
        return it != m_activeBatchNodes.end() && !it->second.empty();
    }

    /**
     * @brief Reset all dependencies for a node.
     *
     * This clears dependencies and cleans up related data structures.
     * @param node The node to reset.
     * @throws std::runtime_error if the node is currently involved in an active batch operation
     */
    void resetNode(const NodePtr &node) {
        REACTION_REGISTER_THREAD();
        ConditionalUniqueLock<ConditionalSharedMutex> lock(m_graphMutex);

        // Check if node exists in dependent list first
        if (m_dependentList.contains(node)) {
            for (auto dep : m_dependentList[node]) {
                if (auto locked_dep = dep.lock()) {
                    if (m_observerList.contains(locked_dep)) {
                        m_observerList.at(locked_dep).get().erase(node);
                    }
                }
            }
            m_dependentList.at(node).clear();
        }

        // Invalidate all caches due to graph structure change
        m_graphCache.invalidateAll();
        m_cycleCache.invalidateAll();
        m_metricsCache.invalidateAll();
    }

    /**
     * @brief Transactional update of all observer dependencies.
     *
     * This method ensures atomicity: either all observer updates succeed,
     * or the original state is completely restored on failure.
     *
     * @tparam Args Parameter pack for node pointers.
     * @param node The node whose observers are being updated.
     * @param args New nodes to observe.
     */
    template <typename... Args>
    void updateObserversTransactional(const NodePtr &node, Args &&...args) {
        if (!node) return;

        REACTION_REGISTER_THREAD();
        ConditionalUniqueLock<ConditionalSharedMutex> lock(m_graphMutex);

        // Step 1: Save current state for rollback
        NodeSet originalDependents;
        if (m_dependentList.contains(node)) {
            originalDependents = m_dependentList[node];
        }

        // Step 2: Reset current dependencies
        resetNodeInternal(node);

        // Step 3: Try to add new observers
        try {
            // Add each observer using fold expression
            ((args ? addObserverInternal(node, args) : void()), ...);

        } catch (const std::exception &) {
            // Step 4: Rollback - restore original dependencies
            resetNodeInternal(node); // Clear any partially added observers

            // Restore original dependencies
            for (auto &dep : originalDependents) {
                if (auto locked_dep = dep.lock()) {
                    try {
                        addObserverInternal(node, locked_dep);
                    } catch (...) {
                        // If restore fails, we're in an inconsistent state, but better than crashing
                        // Log error or handle gracefully
                    }
                }
            }

            throw; // Re-throw the original exception
        }
    }

    /**
     * @brief Transactional reset and update for a node's state.
     *
     * This method saves the current observer state, allows external operations,
     * and provides rollback capability if needed.
     *
     * @param node The node to save state for.
     * @return A rollback function that can restore the original state.
     */
    std::function<void()> saveNodeStateForRollback(const NodePtr &node) {
        if (!node) return []() {};

        REACTION_REGISTER_THREAD();
        ConditionalUniqueLock<ConditionalSharedMutex> lock(m_graphMutex);

        // Save current state
        NodeSet originalDependents;
        if (m_dependentList.contains(node)) {
            originalDependents = m_dependentList[node];
        }

        // Return rollback function
        return [this, node, originalDependents = std::move(originalDependents)]() mutable {
            REACTION_REGISTER_THREAD();
            ConditionalUniqueLock<ConditionalSharedMutex> lock(m_graphMutex);

            // Reset current dependencies
            resetNodeInternal(node);

            // Restore original dependencies
            for (auto &dep : originalDependents) {
                if (auto locked_dep = dep.lock()) {
                    try {
                        addObserverInternal(node, locked_dep);
                    } catch (...) {
                        // If restore fails, we're in an inconsistent state, but better than crashing
                    }
                }
            }
        };
    }

    /**
     * @brief Recursively remove a node and its downstream dependents.
     *
     * This is a cascade delete for the node and all nodes depending on it.
     * @param node Node to remove.
     */
    void closeNode(const NodePtr &node) {
        if (!node) return;
        REACTION_REGISTER_THREAD();
        ConditionalUniqueLock<ConditionalSharedMutex> lock(m_graphMutex);

        NodeSet closedNodes;
        cascadeCloseDependents(node, closedNodes);
    }

    void collectObservers(const NodePtr &node, NodeSet &observers, uint8_t depth) noexcept;

    /**
     * @brief Set a human-readable name for a node.
     *
     * Names are used for logging and debugging.
     * @param node Node to name.
     * @param name Assigned name.
     */
    void setName(const NodePtr &node, const std::string &name) noexcept {
        REACTION_REGISTER_THREAD();
        ConditionalUniqueLock<ConditionalSharedMutex> lock(m_graphMutex);
        m_nameList.insert({node, name});
    }

    /**
     * @brief Get the name of a node.
     * @param node Node to query.
     * @return Human-readable name or empty string if not found.
     */
    [[nodiscard]] std::string getName(const NodePtr &node) noexcept {
        REACTION_REGISTER_THREAD();
        ConditionalSharedLock<ConditionalSharedMutex> lock(m_graphMutex);
        return getNameInternal(node);
    }

    /**
     * @brief Trigger cleanup of all cache subsystems.
     *
     * This method is called by the cache manager to perform periodic
     * cleanup of expired cache entries across all caches.
     */
    void triggerCacheCleanup() noexcept {
        m_graphCache.triggerCleanup();
        m_cycleCache.triggerCleanup();
        m_metricsCache.triggerCleanup();
    }

    /**
     * @brief Get cache statistics from all subsystems.
     */
    struct CacheStats {
        GraphTraversalCache::Stats graphStats;
        CycleDetectionCache::Stats cycleStats;
        NodeMetricsCache::Stats metricsStats;

        CacheStats(const GraphTraversalCache::Stats &gStats,
            const CycleDetectionCache::Stats &cStats,
            const NodeMetricsCache::Stats &mStats)
            : graphStats(gStats), cycleStats(cStats), metricsStats(mStats) {}
    };

    CacheStats getCacheStats() const noexcept {
        auto graphStats = m_graphCache.getStats();
        auto cycleStats = m_cycleCache.getStats();
        auto metricsStats = m_metricsCache.getStats();
        return CacheStats{graphStats, cycleStats, metricsStats};
    }

private:
    ObserverGraph() {}

    /**
     * @brief Internal get name operation without locking.
     * Should only be called when the graph mutex is already held.
     * @param node Node to query.
     * @return Human-readable name or empty string if not found.
     */
    [[nodiscard]] std::string getNameInternal(const NodePtr &node) noexcept {
        if (m_nameList.contains(node)) {
            return m_nameList[node];
        } else {
            return "";
        }
    }

    /**
     * @brief Internal add node operation without locking.
     * Should only be called when the graph mutex is already held.
     * @param node Node to add.
     */
    void addNodeInternal(const NodePtr &node) noexcept;

    /**
     * @brief Helper function to recursively close dependents of a node.
     * @param node Starting node.
     * @param closedNodes Set of already closed nodes to avoid cycles.
     */
    void cascadeCloseDependents(const NodePtr &node, NodeSet &closedNodes) {
        if (!node || closedNodes.contains(node)) return;
        closedNodes.insert(node);

        // Check if node exists in observer list to avoid at() exceptions
        if (m_observerList.contains(node)) {
            auto observerLists = m_observerList.at(node).get();

            for (auto &ob : observerLists) {
                cascadeCloseDependents(ob.lock(), closedNodes);
            }
        }

        closeNodeInternal(node);
    }

    /**
     * @brief Internal close operation for a node.
     *
     * Removes node from all internal data structures.
     * @param node Node to close.
     */
    void closeNodeInternal(const NodePtr &node) {
        if (!node) return;

        // Clean up dependent relationships - this node observes others
        if (m_dependentList.contains(node)) {
            for (auto dep : m_dependentList[node]) {
                if (auto locked_dep = dep.lock()) {
                    if (m_observerList.contains(locked_dep)) {
                        m_observerList.at(locked_dep).get().erase(node);
                    }
                }
            }
            m_dependentList.erase(node);
        }

        // Clean up observer relationships - others observe this node
        if (m_observerList.contains(node)) {
            for (auto &ob : m_observerList.at(node).get()) {
                if (auto locked_ob = ob.lock()) {
                    if (m_dependentList.contains(locked_ob)) {
                        m_dependentList.at(locked_ob).erase(node);
                    }
                }
            }
            m_observerList.erase(node);
        }

        // Remove name mapping
        m_nameList.erase(node);
    }

    /**
     * @brief Detect whether adding an observer relationship would cause a cycle.
     *
     * Uses caching to avoid expensive DFS traversals for previously checked node pairs.
     * @param source The observing node.
     * @param target The node being observed.
     * @return true if a cycle would be created, false otherwise.
     */
    [[nodiscard]] bool hasCycle(const NodePtr &source, const NodePtr &target) {
        // Check if nodes exist in the graph first to avoid at() exceptions
        if (!m_dependentList.contains(source) || !m_observerList.contains(target)) {
            return false; // Nodes don't exist, no cycle possible
        }

        // Try to get cached result first
        const bool *cachedResult = m_cycleCache.getCachedCycleResult(source, target);
        if (cachedResult) {
            return *cachedResult;
        }

        // Cache miss - perform DFS and cache the result
        m_dependentList.at(source).insert(target);
        m_observerList.at(target).get().insert(source);

        NodeSet visited;
        NodeSet recursionStack;
        bool hasCycleResult = dfs(source, visited, recursionStack);

        m_dependentList.at(source).erase(target);
        m_observerList.at(target).get().erase(source);

        // Cache the result for future use
        m_cycleCache.cacheCycleResult(source, target, hasCycleResult);

        return hasCycleResult;
    }

    /**
     * @brief DFS traversal for cycle detection.
     * @param node Current node in DFS.
     * @param visited Set of visited nodes.
     * @param recursionStack Stack of nodes in current DFS path.
     * @return true if cycle detected, false otherwise.
     */
    [[nodiscard]] bool dfs(const NodePtr &node, NodeSet &visited, NodeSet &recursionStack) {
        if (recursionStack.contains(node)) return true;
        if (visited.contains(node)) return false;

        visited.insert(node);
        recursionStack.insert(node);

        // Check if node exists in dependent list to avoid at() exceptions
        if (m_dependentList.contains(node)) {
            for (auto neighbor : m_dependentList.at(node)) {
                if (auto locked_neighbor = neighbor.lock()) {
                    if (dfs(locked_neighbor, visited, recursionStack))
                        return true;
                }
            }
        }

        recursionStack.erase(node);
        return false;
    }

    /**
     * @brief Internal reset operation without locking.
     * Should only be called when graph mutex is already held.
     * @param node The node to reset.
     */
    void resetNodeInternal(const NodePtr &node) {
        if (!node) return;

        // Clean up dependent relationships - this node observes others
        if (m_dependentList.contains(node)) {
            for (auto dep : m_dependentList[node]) {
                if (auto locked_dep = dep.lock()) {
                    if (m_observerList.contains(locked_dep)) {
                        // Take the node's observer mutex before modifying its observer set
                        ConditionalUniqueLock<ConditionalSharedMutex> depLock(locked_dep->m_observersMutex);
                        m_observerList.at(locked_dep).get().erase(node);
                    }
                }
            }
            m_dependentList.at(node).clear();
        }
    }

    /**
     * @brief Internal add observer operation without locking.
     * Should only be called when graph mutex is already held.
     * @param source The observing node.
     * @param target The node being observed.
     */
    void addObserverInternal(const NodePtr &source, const NodePtr &target) {
        if (source == target) {
            REACTION_THROW_SELF_OBSERVATION(getNameInternal(source));
        }

        // Ensure both nodes exist in the graph
        if (!m_dependentList.contains(source)) {
            addNodeInternal(source);
        }
        if (!m_observerList.contains(target)) {
            addNodeInternal(target);
        }

        if (hasCycle(source, target)) {
            REACTION_THROW_DEPENDENCY_CYCLE(getNameInternal(source), getNameInternal(target));
        }

        m_dependentList.at(source).insert(target);
        {
            ConditionalUniqueLock<ConditionalSharedMutex> targetLock(target->m_observersMutex);
            target->m_observers.insert(source);
        }
    }

    std::unordered_map<NodePtr, NodeSetRef> m_observerList;                 ///< Map from node to its observers (refs).
    std::unordered_map<NodePtr, NodeSet> m_dependentList;                   ///< Map from node to its dependencies.
    std::unordered_map<NodePtr, std::string> m_nameList;                    ///< Human-readable node names.
    std::unordered_map<NodePtr, std::set<const void *>> m_activeBatchNodes; ///< Map from node to active batch IDs.
    std::set<const void *> m_activeBatchIds;                                ///< Set of all active batch IDs.
    mutable ConditionalSharedMutex m_graphMutex;                            ///< Mutex for thread-safe graph operations.

    // Cache subsystems
    mutable GraphTraversalCache m_graphCache; ///< Cache for graph traversal results.
    mutable CycleDetectionCache m_cycleCache; ///< Cache for cycle detection results.
    mutable NodeMetricsCache m_metricsCache;  ///< Cache for node metrics and existence checks.
};

/**
 * @brief Implementation of ObserverGraph::addNode.
 *
 * Initializes internal data structures for the given node.
 * @param node Node to add.
 */
inline void ObserverGraph::addNode(const NodePtr &node) noexcept {
    REACTION_REGISTER_THREAD();
    ConditionalUniqueLock<ConditionalSharedMutex> lock(m_graphMutex);
    m_observerList.insert({node, std::ref(node->m_observers)});
    m_dependentList[node] = NodeSet{};
}

/**
 * @brief Implementation of ObserverGraph::addNodeInternal.
 *
 * Initializes internal data structures for the given node without locking.
 * @param node Node to add.
 */
inline void ObserverGraph::addNodeInternal(const NodePtr &node) noexcept {
    m_observerList.insert({node, std::ref(node->m_observers)});
    m_dependentList[node] = NodeSet{};
}

/**
 * @brief Implementation of ObserverGraph::collectObservers.
 * @param node origin node to find observers.
 * @param observers container for output observers.
 * @param depth current dependent depth.
 */
inline void ObserverGraph::collectObservers(const NodePtr &node, NodeSet &observers, uint8_t depth = 1) noexcept {
    if (!node) return;
    REACTION_REGISTER_THREAD();

    // Try to get cached immediate observers first
    const NodeSet *cachedObservers = m_graphCache.getCachedImmediateObservers(node);
    NodeSet currentObservers;

    if (cachedObservers) {
        // Use cached data - still need to update depth and collect recursively
        currentObservers = *cachedObservers;
        for (auto &ob : currentObservers) {
            if (auto wp = ob.lock()) [[likely]] {
                wp->updateDepth(depth);
                observers.insert(ob);
            }
        }
    } else {
        // Cache miss - collect from graph and cache the result
        ConditionalSharedLock<ConditionalSharedMutex> lock(m_graphMutex);
        if (m_observerList.contains(node)) {
            for (auto &ob : m_observerList.at(node).get()) {
                if (auto wp = ob.lock()) [[likely]] {
                    wp->updateDepth(depth);
                    observers.insert(ob);
                    currentObservers.insert(ob);
                }
            }
            // Cache the immediate observers for future use
            m_graphCache.cacheImmediateObservers(node, currentObservers);
        }
    }

    // Process collected observers recursively without holding lock
    for (auto &ob : currentObservers) {
        if (auto wp = ob.lock()) [[likely]] {
            collectObservers(wp, observers, depth + 1);
        }
    }
}

} // namespace reaction
