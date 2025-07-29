/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/utility.h"

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
        if (source == target) {
            std::ostringstream oss;
            oss << "detect observe self, node name = " << m_nameList[source];
            throw std::runtime_error(oss.str());
        }
        if (hasCycle(source, target)) {
            std::ostringstream oss;
            oss << "detect cycle dependency, source name = " << m_nameList[source] << " target name = " << m_nameList[target];
            throw std::runtime_error(oss.str());
        }

        m_dependentList.at(source).insert(target);
        m_observerList.at(target).get().insert({source});
    }

    /**
     * @brief Reset all dependencies for a node.
     *
     * This clears dependencies and cleans up related data structures.
     * @param node The node to reset.
     */
    void resetNode(const NodePtr &node) {
        for (auto dep : m_dependentList[node]) {
            m_observerList.at(dep.lock()).get().erase(node);
        }
        m_dependentList.at(node).clear();
    }

    /**
     * @brief Recursively remove a node and its downstream dependents.
     *
     * This is a cascade delete for the node and all nodes depending on it.
     * @param node Node to remove.
     */
    void closeNode(const NodePtr &node) {
        if (!node) return;
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
        m_nameList.insert({node, name});
    }

    /**
     * @brief Get the name of a node.
     * @param node Node to query.
     * @return Human-readable name or empty string if not found.
     */
    [[nodiscard]] std::string getName(const NodePtr &node) noexcept {
        if (m_nameList.contains(node)) {
            return m_nameList[node];
        } else {
            return "";
        }
    }

private:
    ObserverGraph() {}

    /**
     * @brief Helper function to recursively close dependents of a node.
     * @param node Starting node.
     * @param closedNodes Set of already closed nodes to avoid cycles.
     */
    void cascadeCloseDependents(const NodePtr &node, NodeSet &closedNodes) {
        if (!node || closedNodes.contains(node)) return;
        closedNodes.insert(node);
        auto observerLists = m_observerList.at(node).get();

        for (auto &ob : observerLists) {
            cascadeCloseDependents(ob.lock(), closedNodes);
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
        for (auto dep : m_dependentList[node]) {
            m_observerList.at(dep.lock()).get().erase(node);
        }
        m_dependentList.erase(node);
        for (auto &ob : m_observerList.at(node).get()) {
            m_dependentList.at(ob.lock()).erase(node);
        }
        m_observerList.erase(node);
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

        for (auto neighbor : m_dependentList.at(node)) {
            if (dfs(neighbor.lock(), visited, recursionStack))
                return true;
        }

        recursionStack.erase(node);
        return false;
    }

    std::unordered_map<NodePtr, NodeSetRef> m_observerList; ///< Map from node to its observers (refs).
    std::unordered_map<NodePtr, NodeSet> m_dependentList;   ///< Map from node to its dependencies.
    std::unordered_map<NodePtr, std::string> m_nameList;    ///< Human-readable node names.
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

    virtual void changedNoNotify([[maybe_unused]] bool changed = true) {}

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
        ObserverGraph::getInstance().resetNode(shared_this);
        (ObserverGraph::getInstance().addObserver(shared_this, args), ...);
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
        m_fieldMap[id].insert(node);
    }

    /**
     * @brief Remove all fields for an object.
     * @param id Object ID.
     */
    void deleteObj(uint64_t id) noexcept {
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
        if (!m_fieldMap.contains(id)) {
            return;
        }
        for (auto &node : m_fieldMap[id]) {
            ObserverGraph::getInstance().addObserver(objPtr, node.lock());
        }
    }

private:
    FieldGraph() {}
    std::unordered_map<uint64_t, NodeSet> m_fieldMap; ///< Map from object ID to its field nodes.
};

/**
 * @brief Implementation of ObserverGraph::addNode.
 *
 * Initializes internal data structures for the given node.
 * @param node Node to add.
 */
inline void ObserverGraph::addNode(const NodePtr &node) noexcept {
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

    for (auto &ob : m_observerList.at(node).get()) {
        if (auto wp = ob.lock()) [[likely]] {
            wp->updateDepth(depth);
            observers.insert(ob);
            collectObservers(wp, observers, ++depth);
        }
    }
}

} // namespace reaction
