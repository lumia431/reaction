/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/utility.h"
#include "reaction/thread_safety.h"
#include <functional>
#include <set>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace reaction {

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
        ConditionalUniqueLock lock(m_graphMutex);

        if (source == target) {
            std::ostringstream oss;
            oss << "detect observe self, node name = " << getNameInternal(source);
            throw std::runtime_error(oss.str());
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
            std::ostringstream oss;
            oss << "detect cycle dependency, source name = " << getNameInternal(source)
                << " target name = " << getNameInternal(target);
            throw std::runtime_error(oss.str());
        }

        m_dependentList.at(source).insert(target);
        m_observerList.at(target).get().insert({source});
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
    void registerActiveBatch(const void* batchId, const NodeSet& nodes) {
        ConditionalUniqueLock lock(m_graphMutex);
        for (const auto& nodeWeak : nodes) {
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
    void unregisterActiveBatch(const void* batchId) {
        ConditionalUniqueLock lock(m_graphMutex);
        m_activeBatchIds.erase(batchId);

        // Remove this batch from all node tracking
        for (auto it = m_activeBatchNodes.begin(); it != m_activeBatchNodes.end(); ) {
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
    bool isNodeInActiveBatch(const NodePtr& node) const {
        ConditionalSharedLock lock(m_graphMutex);
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
        ConditionalUniqueLock lock(m_graphMutex);

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
        ConditionalUniqueLock lock(m_graphMutex);

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
        ConditionalUniqueLock lock(m_graphMutex);

        // Save current state
        NodeSet originalDependents;
        if (m_dependentList.contains(node)) {
            originalDependents = m_dependentList[node];
        }

        // Return rollback function
        return [this, node, originalDependents = std::move(originalDependents)]() mutable {
            REACTION_REGISTER_THREAD();
            ConditionalUniqueLock lock(m_graphMutex);

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
        ConditionalUniqueLock lock(m_graphMutex);

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
        ConditionalUniqueLock lock(m_graphMutex);
        m_nameList.insert({node, name});
    }

    /**
     * @brief Get the name of a node.
     * @param node Node to query.
     * @return Human-readable name or empty string if not found.
     */
    [[nodiscard]] std::string getName(const NodePtr &node) noexcept {
        REACTION_REGISTER_THREAD();
        ConditionalSharedLock lock(m_graphMutex);
        return getNameInternal(node);
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
     * Temporarily inserts the edge, performs DFS cycle detection, then removes edge.
     * @param source The observing node.
     * @param target The node being observed.
     * @return true if a cycle would be created, false otherwise.
     */
    [[nodiscard]] bool hasCycle(const NodePtr &source, const NodePtr &target) {
        // Check if nodes exist in the graph first to avoid at() exceptions
        if (!m_dependentList.contains(source) || !m_observerList.contains(target)) {
            return false; // Nodes don't exist, no cycle possible
        }

        m_dependentList.at(source).insert(target);
        m_observerList.at(target).get().insert(source);

        NodeSet visited;
        NodeSet recursionStack;
        bool hasCycle = dfs(source, visited, recursionStack);

        m_dependentList.at(source).erase(target);
        m_observerList.at(target).get().erase(source);

        return hasCycle;
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
            std::ostringstream oss;
            oss << "detect observe self, node name = " << getNameInternal(source);
            throw std::runtime_error(oss.str());
        }

        // Ensure both nodes exist in the graph
        if (!m_dependentList.contains(source)) {
            addNodeInternal(source);
        }
        if (!m_observerList.contains(target)) {
            addNodeInternal(target);
        }

        if (hasCycle(source, target)) {
            std::ostringstream oss;
            oss << "detect cycle dependency, source name = " << getNameInternal(source)
                << " target name = " << getNameInternal(target);
            throw std::runtime_error(oss.str());
        }

        m_dependentList.at(source).insert(target);
        m_observerList.at(target).get().insert(source);
    }

    std::unordered_map<NodePtr, NodeSetRef> m_observerList; ///< Map from node to its observers (refs).
    std::unordered_map<NodePtr, NodeSet> m_dependentList;   ///< Map from node to its dependencies.
    std::unordered_map<NodePtr, std::string> m_nameList;    ///< Human-readable node names.
    std::unordered_map<NodePtr, std::set<const void*>> m_activeBatchNodes; ///< Map from node to active batch IDs.
    std::set<const void*> m_activeBatchIds;                ///< Set of all active batch IDs.
    mutable ConditionalSharedMutex m_graphMutex;            ///< Mutex for thread-safe graph operations.
};

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
    void updateObservers(auto &&...args) {
        auto shared_this = shared_from_this();
        ObserverGraph::getInstance().updateObserversTransactional(shared_this, args...);
    }

    /**
     * @brief Add a single observer node.
     * @param node Observer node to add.
     */
    void addOneObserver(const NodePtr &node) {
        ObserverGraph::getInstance().addObserver(shared_from_this(), node);
    }

    /**
     * @brief Notify observers and delayed repeat nodes.
     * @param changed Whether the node's value has changed.
     */
    void notify(bool changed = true) {
        for (auto &observer : m_observers) {
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

/**
 * @brief Manages object field bindings in the reactive graph.
 *
 * Used to track field-depth reactive nodes for specific object IDs.
 */
class FieldGraph {
public:
    /**
     * @brief Get singleton instance.
     * @return FieldGraph& singleton reference.
     */
    [[nodiscard]] static FieldGraph &getInstance() noexcept {
        static FieldGraph instance;
        return instance;
    }

    /**
     * @brief Register a node under an object ID.
     * @param id Object ID.
     * @param node Node pointer.
     */
    void addObj(uint64_t id, const NodePtr &node) noexcept {
        REACTION_REGISTER_THREAD();
        ConditionalUniqueLock lock(m_fieldMutex);
        m_fieldMap[id].insert(node);
    }

    /**
     * @brief Remove all fields for an object.
     * @param id Object ID.
     */
    void deleteObj(uint64_t id) noexcept {
        REACTION_REGISTER_THREAD();
        ConditionalUniqueLock lock(m_fieldMutex);
        m_fieldMap.erase(id);
    }

    /**
     * @brief Bind all field nodes of an object to an input node.
     *
     * Adds observer edges from objPtr to each field node under the object ID.
     * @param id Object ID.
     * @param objPtr Source node.
     */
    void bindField(uint64_t id, const NodePtr &objPtr) {
        REACTION_REGISTER_THREAD();
        NodeSet nodesToBind;
        {
            ConditionalSharedLock lock(m_fieldMutex);
            if (!m_fieldMap.contains(id)) {
                return;
            }
            nodesToBind = m_fieldMap[id]; // Copy the set
        }

        // Bind nodes without holding the field mutex to avoid deadlock with ObserverGraph
        for (auto &node : nodesToBind) {
            ObserverGraph::getInstance().addObserver(objPtr, node.lock());
        }
    }

private:
    FieldGraph() {}
    std::unordered_map<uint64_t, NodeSet> m_fieldMap; ///< Map from object ID to its field nodes.
    mutable ConditionalSharedMutex m_fieldMutex;       ///< Mutex for thread-safe field operations.
};

/**
 * @brief Implementation of ObserverGraph::addNode.
 *
 * Initializes internal data structures for the given node.
 * @param node Node to add.
 */
inline void ObserverGraph::addNode(const NodePtr &node) noexcept {
    REACTION_REGISTER_THREAD();
    ConditionalUniqueLock lock(m_graphMutex);
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

    // Use a lock-free approach for observer collection to avoid recursive locking
    // We collect current observers first, then process them
    NodeSet currentObservers;
    {
        ConditionalSharedLock lock(m_graphMutex);
        for (auto &ob : m_observerList.at(node).get()) {
            if (auto wp = ob.lock()) [[likely]] {
                wp->updateDepth(depth);
                observers.insert(ob);
                currentObservers.insert(ob);
            }
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
