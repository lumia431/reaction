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
 * @brief Global thread-local set used for delaying node notifications.
 *
 * This set is used to avoid recursive notifications and manage delayed updates.
 */
inline thread_local NodeSet g_delay_list;

/**
 * @brief Manages the graph structure of reactive observers and dependencies.
 *
 * This singleton class is responsible for:
 * - Adding and removing nodes
 * - Managing observer/dependent relationships
 * - Detecting cycles
 * - Managing repeated dependencies
 */
class ObserverGraph {
public:
    /**
     * @brief Get the singleton instance of the graph.
     * @return ObserverGraph& singleton reference.
     */
    static ObserverGraph &getInstance() {
        static ObserverGraph instance;
        return instance;
    }

    /**
     * @brief Add a new node into the graph.
     * @param node Node to add.
     */
    void addNode(NodePtr node);

    /**
     * @brief Add an observer relationship (source observes target).
     *
     * @param source The observing node.
     * @param target The node being observed.
     * @throws std::runtime_error if a cycle or self-observation is detected.
     */
    void addObserver(NodePtr source, NodePtr target) {
        if (source == target) {
            Log::error("Cannot observe self, node = {}.", m_nameList[source]);
            throw std::runtime_error("detect observe self");
        }
        if (hasCycle(source, target)) {
            Log::error("Cycle dependency detected, node = {}. Cycle dependent = {}", m_nameList[source], m_nameList[target]);
            throw std::runtime_error("detect cycle dependency");
        }

        hasRepeatDependencies(source, target);

        m_dependentList.at(source).insert(target);
        m_observerList.at(target).get().insert({source});
    }

    /**
     * @brief Reset all dependencies for a node.
     *
     * This clears dependencies and cleans up related data structures.
     * @param node The node to reset.
     */
    void resetNode(NodePtr node) {
        cleanupDependencies(node);
        m_dependentList.at(node).clear();
    }

    /**
     * @brief Recursively remove a node and its downstream dependents.
     *
     * This is a cascade delete for the node and all nodes depending on it.
     * @param node Node to remove.
     */
    void closeNode(NodePtr node) {
        if (!node) return;
        NodeSet closedNodes;
        cascadeCloseDependents(node, closedNodes);
    }

    /**
     * @brief Set a human-readable name for a node.
     *
     * Names are used for logging and debugging.
     * @param node Node to name.
     * @param name Assigned name.
     */
    void setName(NodePtr node, const std::string &name) {
        m_nameList.insert({node, name});
    }

    /**
     * @brief Get the name of a node.
     * @param node Node to query.
     * @return Human-readable name or empty string if not found.
     */
    std::string getName(NodePtr node) {
        if (m_nameList.contains(node)) {
            return m_nameList[node];
        } else {
            return "";
        }
    }

private:
    ObserverGraph() {}

    /**
     * @brief Cleanup dependency data related to a node.
     *
     * This handles removing observer links and adjusting repeated dependency counts.
     * @param node Node whose dependencies are cleaned up.
     */
    void cleanupDependencies(NodePtr node) {
        NodeSet observers;
        collectObservers(node, observers);

        NodeSet dependents;
        collectDependencies(node, dependents);

        for (const auto &weak_dep : dependents) {
            const auto dep = weak_dep.lock();
            if (!dep) continue;

            if (auto repeat_it = m_repeatList.find(dep); repeat_it != m_repeatList.end()) {
                auto &repeat_map = repeat_it->second.get();

                for (auto map_it = repeat_map.begin(); map_it != repeat_map.end();) {
                    Log::info("reduce repeat dependent, source = {}, repeatNode = {}, count = {}", m_nameList[dep], m_nameList[map_it->first.lock()], map_it->second);
                    if (observers.contains(map_it->first) && --map_it->second == 1) {
                        map_it = repeat_map.erase(map_it);
                    } else {
                        ++map_it;
                    }
                }
            }
        }

        for (auto dep : m_dependentList[node]) {
            m_observerList.at(dep.lock()).get().erase(node);
        }
    }

    /**
     * @brief Helper function to recursively close dependents of a node.
     * @param node Starting node.
     * @param closedNodes Set of already closed nodes to avoid cycles.
     */
    void cascadeCloseDependents(NodePtr node, NodeSet &closedNodes) {
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
    void closeNodeInternal(NodePtr node) {
        if (!node) return;
        for (auto dep : m_dependentList[node]) {
            m_observerList.at(dep.lock()).get().erase(node);
        }
        m_dependentList.erase(node);
        for (auto &ob : m_observerList.at(node).get()) {
            m_dependentList.at(ob.lock()).erase(node);
        }
        m_observerList.erase(node);
        m_repeatList.erase(node);
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
    bool hasCycle(NodePtr source, NodePtr target) {
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
    bool dfs(NodePtr node, NodeSet &visited, NodeSet &recursionStack) {
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

    /**
     * @brief Detect repeated dependencies and update tracking.
     * @param source The observing node.
     * @param target The node being observed.
     */
    void hasRepeatDependencies(NodePtr source, NodePtr target) {

        NodeSet dependencies;
        collectDependencies(target, dependencies);

        NodeSet visited;
        for (auto &dependent : m_dependentList.at(source)) {
            checkDependency(source, dependent.lock(), dependencies, visited);
        }
    }

    /**
     * @brief Collect all dependencies of a node, counting occurrences.
     * @param node Node to analyze.
     * @param dependencies Map from node to count of dependencies.
     */
    void collectDependencies(NodePtr node, NodeMap &dependencies) {
        if (!node) return;

        if (dependencies.count(node)) {
            dependencies[node]++;
        } else {
            dependencies[node] = 1;
        }

        for (auto neighbor : m_dependentList.at(node)) {
            collectDependencies(neighbor.lock(), dependencies);
        }
    }

    /**
     * @brief Collect unique dependencies of a node into a set.
     * @param node Node to analyze.
     * @param dependencies Set to fill with unique dependencies.
     */
    void collectDependencies(NodePtr node, NodeSet &dependencies) {
        NodeMap targetDependencies;
        collectDependencies(node, targetDependencies);

        for (auto &[dependent, count] : targetDependencies) {
            if (count == 1) {
                dependencies.insert(dependent);
            }
        }
    }

    /**
     * @brief Collect all observers of a node, counting occurrences.
     * @param node Node to analyze.
     * @param observers Map from node to count of observers.
     */
    void collectObservers(NodePtr node, NodeMap &observers) {
        if (!node) return;

        if (observers.contains(node)) {
            observers[node]++;
        } else {
            observers[node] = 1;
        }

        for (auto neighbor : m_observerList.at(node).get()) {
            collectObservers(neighbor.lock(), observers);
        }
    }

    /**
     * @brief Collect unique observers of a node into a set.
     * @param node Node to analyze.
     * @param observers Set to fill with unique observers.
     */
    void collectObservers(NodePtr node, NodeSet &observers) {
        NodeMap targetObservers;
        collectDependencies(node, targetObservers);

        for (auto &[observer, count] : targetObservers) {
            if (count == 1) {
                observers.insert(observer);
            }
        }
    }

    /**
     * @brief Check if a node is a repeated dependency and update repeat counts.
     * @param source Observing node.
     * @param node Current node being checked.
     * @param targetDependencies Set of dependencies to check against.
     * @param visited Set of nodes already visited to prevent cycles.
     */
    void checkDependency(NodePtr source, NodePtr node, NodeSet &targetDependencies, NodeSet &visited) {
        if (visited.contains(node)) return;
        visited.insert(node);

        if (targetDependencies.contains(node)) {
            Log::info("detect repeat dependent, source = {}, repeatNode = {}.", m_nameList[source], m_nameList[node]);
            if (m_repeatList.at(node).get().contains(source)) {
                m_repeatList.at(node).get()[source]++;
            } else {
                m_repeatList.at(node).get()[source] = 2;
            }
        }

        for (auto &dependent : m_dependentList.at(node)) {
            checkDependency(source, dependent.lock(), targetDependencies, visited);
        }
    }

    std::unordered_map<NodePtr, NodeSetRef> m_observerList;   ///< Map from node to its observers (refs).
    std::unordered_map<NodePtr, NodeSet> m_dependentList;     ///< Map from node to its dependencies.
    std::unordered_map<NodePtr, NodeMapRef> m_repeatList;     ///< Repeated dependencies tracking.
    std::unordered_map<NodePtr, std::string> m_nameList;      ///< Human-readable node names.
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
    void addOneObserver(NodePtr node) {
        ObserverGraph::getInstance().addObserver(shared_from_this(), node);
    }

    /**
     * @brief Notify observers and delayed repeat nodes.
     *
     * Notifies all observers unless already delayed, then processes delayed repeat nodes.
     * @param changed Whether the node's value has changed.
     */
    void notify(bool changed = true) {
        // Log::info("node {} trigger.", ObserverGraph::getInstance().getName(shared_from_this()));

        // Insert repeat nodes into delay list to prevent re-entrant notifications.
        for (auto &[repeat, _] : m_repeats) {
            g_delay_list.insert(repeat);
        }

        // Notify observers not in delay list.
        for (auto &observer : m_observers) {
            if (!g_delay_list.contains(observer)) {
                if (auto wp = observer.lock()) wp->valueChanged(changed);
            }
        }

        // Notify delayed repeat nodes.
        if (!g_delay_list.empty()) {
            for (auto &[repeat, _] : m_repeats) {
                g_delay_list.erase(repeat);
            }
            for (auto &[repeat, _] : m_repeats) {
                if (auto wp = repeat.lock()) wp->valueChanged(changed);
            }
        }
    }

private:
    NodeSet m_observers;  ///< Direct observers of this node.
    NodeMap m_repeats;    ///< Repeat observers and their counts.
    friend class ObserverGraph;
};

/**
 * @brief Manages object field bindings in the reactive graph.
 *
 * Used to track field-level reactive nodes for specific object IDs.
 */
class FieldGraph {
public:
    /**
     * @brief Get singleton instance.
     * @return FieldGraph& singleton reference.
     */
    static FieldGraph &getInstance() {
        static FieldGraph instance;
        return instance;
    }

    /**
     * @brief Register a node under an object ID.
     * @param id Object ID.
     * @param node Node pointer.
     */
    void addObj(uint64_t id, NodePtr node) {
        m_fieldMap[id].insert(node);
    }

    /**
     * @brief Remove all fields for an object.
     * @param id Object ID.
     */
    void deleteObj(uint64_t id) {
        m_fieldMap.erase(id);
    }

    /**
     * @brief Bind all field nodes of an object to an input node.
     *
     * Adds observer edges from objPtr to each field node under the object ID.
     * @param id Object ID.
     * @param objPtr Source node.
     */
    void bindField(uint64_t id, NodePtr objPtr) {
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
inline void ObserverGraph::addNode(NodePtr node) {
    m_observerList.insert({node, std::ref(node->m_observers)});
    m_dependentList[node] = NodeSet{};
    m_repeatList.insert({node, std::ref(node->m_repeats)});
}

} // namespace reaction
