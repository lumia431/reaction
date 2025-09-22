/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/cache/cache_base.h"
#include "reaction/core/types.h"

namespace reaction {

// Forward declaration
class ObserverNode;

/**
 * @brief Lightweight caching for observer graph traversal optimization.
 *
 * This cache is designed specifically for the collectObservers operation
 * and DOES NOT interfere with the updateDepth mechanism or batch operations.
 * It only caches the intermediate graph structure relationships, not the
 * complete traversal results that require depth updates.
 */
class GraphTraversalCache : public PtrCacheBase<NodePtr, NodeSet> {
public:
    GraphTraversalCache() : PtrCacheBase(MAX_CACHE_SIZE, CACHE_TTL) {}

    /**
     * @brief Try to get cached immediate observers for a node.
     *
     * This only returns the direct observers of a node, not the full traversal.
     * The caller still needs to perform the recursive traversal and depth updates.
     *
     * @param node The node to get immediate observers for
     * @return Pointer to cached observers if valid, nullptr otherwise
     */
    const NodeSet* getCachedImmediateObservers(const NodePtr& node) const noexcept {
        return getCachedValue(node);
    }

    /**
     * @brief Cache immediate observers for a node.
     *
     * @param node The node whose immediate observers to cache
     * @param observers The immediate observers (not full traversal)
     */
    void cacheImmediateObservers(const NodePtr& node, const NodeSet& observers) noexcept {
        cacheValue(node, observers);
    }

    /**
     * @brief Invalidate all cache entries due to graph structure change.
     */
    void invalidateAll() noexcept { invalidateAllEntries(); }

    /**
     * @brief Get cache statistics.
     */
    Stats getStats() const noexcept { return getStatsInternal(); }

    /**
     * @brief Trigger cleanup of expired cache entries.
     */
    void triggerCleanup() noexcept { triggerCleanupInternal(); }

private:
    static constexpr size_t MAX_CACHE_SIZE = 500; // Smaller cache for immediate observers only
    static constexpr std::chrono::minutes CACHE_TTL{5}; // Shorter TTL
};

/**
 * @brief Cache for cycle detection results to avoid expensive DFS operations.
 *
 * Caches the results of cycle detection between node pairs, significantly
 * reducing the cost of addObserver operations by avoiding repeated DFS traversals.
 */
class CycleDetectionCache : public PairCacheBase<NodePtr, NodePtr, bool> {
public:
    CycleDetectionCache() : PairCacheBase(MAX_CYCLE_CACHE_SIZE, CACHE_TTL) {}

    /**
     * @brief Try to get cached cycle detection result for a node pair.
     *
     * @param source The observing node
     * @param target The node being observed
     * @return Pointer to cached result if valid, nullptr otherwise
     */
    const bool* getCachedCycleResult(const NodePtr& source, const NodePtr& target) const noexcept {
        return getCachedValue(std::make_pair(source, target));
    }

    /**
     * @brief Cache cycle detection result for a node pair.
     *
     * @param source The observing node
     * @param target The node being observed
     * @param hasCycle Whether adding this edge would create a cycle
     */
    void cacheCycleResult(const NodePtr& source, const NodePtr& target, bool hasCycle) noexcept {
        cacheValue(std::make_pair(source, target), hasCycle);
    }

    /**
     * @brief Invalidate all cache entries due to graph structure change.
     */
    void invalidateAll() noexcept { invalidateAllEntries(); }

    /**
     * @brief Get cache statistics.
     */
    Stats getStats() const noexcept { return getStatsInternal(); }

    /**
     * @brief Trigger cleanup of expired cache entries.
     */
    void triggerCleanup() noexcept { triggerCleanupInternal(); }

private:
    static constexpr size_t MAX_CYCLE_CACHE_SIZE = 1000;
    static constexpr std::chrono::minutes CACHE_TTL{10};
};

/**
 * @brief Cached metrics for a node.
 */
struct NodeMetrics {
    bool exists;           ///< Whether the node exists in the graph
    size_t observerCount;  ///< Number of direct observers
    size_t dependentCount; ///< Number of direct dependencies
    uint8_t maxDepth;      ///< Maximum depth in the dependency tree

    NodeMetrics(bool exists, size_t obsCount, size_t depCount, uint8_t depth)
        : exists(exists), observerCount(obsCount), dependentCount(depCount), maxDepth(depth) {}
};

/**
 * @brief Cache for node existence checks and topology metrics.
 *
 * Caches frequently accessed node properties to reduce map lookup overhead
 * and expensive topology calculations.
 */
class NodeMetricsCache : public PtrCacheBase<NodePtr, NodeMetrics> {
public:
    using Base = PtrCacheBase<NodePtr, NodeMetrics>;

    NodeMetricsCache() : Base(MAX_METRICS_CACHE_SIZE, CACHE_TTL) {}

    /**
     * @brief Try to get cached node existence result.
     *
     * @param node The node to check
     * @return Pointer to cached existence result if valid, nullptr otherwise
     */
    const bool* getCachedNodeExists(const NodePtr& node) const noexcept {
        if (const auto* metrics = this->getCachedValue(node)) {
            return &metrics->exists;
        }
        return nullptr;
    }

    /**
     * @brief Try to get cached node metrics.
     *
     * @param node The node to get metrics for
     * @return Pointer to cached metrics if valid, nullptr otherwise
     */
    const NodeMetrics* getCachedNodeMetrics(const NodePtr& node) const noexcept {
        return this->getCachedValue(node);
    }

    /**
     * @brief Cache node metrics.
     *
     * @param node The node to cache metrics for
     * @param exists Whether the node exists in the graph
     * @param observerCount Number of direct observers
     * @param dependentCount Number of direct dependencies
     * @param maxDepth Maximum depth in the dependency tree
     */
    void cacheNodeMetrics(const NodePtr& node,
                          bool exists,
                          size_t observerCount,
                          size_t dependentCount,
                          uint8_t maxDepth) noexcept {
        this->cacheValue(node, NodeMetrics{exists, observerCount, dependentCount, maxDepth});
    }

    /**
     * @brief Invalidate all cache entries due to graph structure change.
     */
    void invalidateAll() noexcept { this->invalidateAllEntries(); }

    /**
     * @brief Get cache statistics.
     */
    typename Base::Stats getStats() const noexcept { return this->getStatsInternal(); }

    /**
     * @brief Trigger cleanup of expired cache entries.
     */
    void triggerCleanup() noexcept { this->triggerCleanupInternal(); }

private:
    static constexpr size_t MAX_METRICS_CACHE_SIZE = 800;
    static constexpr std::chrono::minutes CACHE_TTL{8};
};

} // namespace reaction