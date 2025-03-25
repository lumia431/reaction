#ifndef REACTION_OBSERVERNODE_H
#define REACTION_OBSERVERNODE_H

#include "reaction/concept.h"
#include "reaction/log.h"
#include <unordered_set>
#include <unordered_map>
#include <functional>

namespace reaction {

// Forward declarations for different node types
struct DataNode {};
struct ActionNode {};
struct FieldNode {};

using DataNodePtr = std::shared_ptr<ObserverDataNode>;
using ActionNodePtr = std::shared_ptr<ObserverActionNode>;
using FieldNodePtr = std::shared_ptr<ObserverFieldNode>;

// ObserverGraph handles dependencies between nodes and manages observers
class ObserverGraph {
public:
    // Singleton instance of ObserverGraph
    static ObserverGraph &getInstance() {
        static ObserverGraph instance;
        return instance;
    }

    // Add a new node to the graph
    template <NodeCC NodeType>
    void addNode(std::shared_ptr<NodeType> node) {
        if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>) {
            m_dataDependents[node] = std::unordered_set<DataNodePtr>();
            m_dataObservers[node] = std::unordered_map<DataNodePtr, std::function<void(bool)>>();
            m_actionObservers[node] = std::unordered_map<ActionNodePtr, std::function<void(bool)>>();
        } else {
            m_actionDependents[node] = std::unordered_set<DataNodePtr>();
        }
    }

    // Add an observer for a specific node
    template <NodeCC NodeType, NodeCC TargetType>
    bool addObserver(bool &repeat, std::function<void(bool)> &f, std::shared_ptr<NodeType> source, std::shared_ptr<TargetType> target) {
        if constexpr (std::is_same_v<NodeType, TargetType>) {
            if (source == target) {
                Log::error("Cannot observe self, node = {}.", source->getName());
                return false;
            }
        }

        if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>) {
            if (hasCycle(source, target)) {
                Log::error("Cycle dependency detected, node = {}. Cycle dependent = {}", source->getName(), target->getName());
                return false;
            }
        }

        if (!repeat && hasRepeatDependencies(source, target)) {
            Log::info("Repeat dependency detected, node = {}. Repeat dependent = {}", source->getName(), target->getName());
            repeat = true;
        }

        if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>) {
            m_dataDependents[source].insert(target);
            m_dataObservers[target].insert({source, f});
        } else {
            m_actionDependents[source].insert(target);
            m_actionObservers[target].insert({source, f});
        }
        return true;
    }

    // Reset the observers for a node
    template <NodeCC NodeType>
    void resetNode(std::shared_ptr<NodeType> node) {
        if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>) {
            for (auto dep : m_dataDependents[node]) {
                m_dataObservers[dep].erase(node);
            }
            m_dataDependents[node].clear();
        } else {
            for (auto dep : m_actionDependents[node]) {
                m_actionObservers[dep].erase(node);
            }
            m_actionDependents[node].clear();
        }
    }

    // Close a node and its dependencies
    template <NodeCC NodeType>
    void closeNode(std::shared_ptr<NodeType> node) {
        if (!node) return;
        if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode> || std::is_same_v<typename NodeType::SourceType, ActionNode>) {
            std::unordered_set<std::shared_ptr<void>> closedNodes;
            cascadeCloseDependents(node, closedNodes);
        }
    }

    // Delete a node from the graph
    template <NodeCC NodeType>
    void deleteNode(std::shared_ptr<NodeType> node) {
        if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>) {
            m_dataDependents.erase(node);
            m_dataObservers.erase(node);
            m_actionObservers.erase(node);
        } else {
            m_actionDependents.erase(node);
        }
    }

    // Get the list of observers for a DataNode
    std::unordered_map<DataNodePtr, std::function<void(bool)>> getDataObserverList(DataNodePtr node) {
        return m_dataObservers[node];
    }

    // Get the list of observers for an ActionNode
    std::unordered_map<ActionNodePtr, std::function<void(bool)>> getActionObserverList(DataNodePtr node) {
        return m_actionObservers[node];
    }

private:
    ObserverGraph() {
    }

    // Cascade close all dependent nodes
    template <NodeCC NodeType>
    void cascadeCloseDependents(std::shared_ptr<NodeType> node, std::unordered_set<std::shared_ptr<void>> &closedNodes) {
        if (!node || closedNodes.count(node)) return;
        closedNodes.insert(node);

        if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>) {
            auto dataNode = std::static_pointer_cast<ObserverDataNode>(node);
            auto dataObservers = m_dataObservers[dataNode];
            auto actionObservers = m_actionObservers[dataNode];

            for (auto &[ob, fun] : dataObservers) {
                cascadeCloseDependents(ob, closedNodes);
            }
            for (auto &[ob, fun] : actionObservers) {
                cascadeCloseDependents(ob, closedNodes);
            }
        }

        closeNodeInternal(node);
    }

    // Internal close node implementation (without cascading)
    template <NodeCC NodeType>
    void closeNodeInternal(std::shared_ptr<NodeType> node) {
        if (!node) return;

        if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>) {
            for (auto &dep : m_dataDependents[node]) {
                m_dataObservers[dep].erase(node);
            }
            m_dataDependents.erase(node);

            for (auto &[ob, fun] : m_dataObservers[node]) {
                m_dataDependents[ob].erase(node);
            }
            for (auto &[ob, fun] : m_actionObservers[node]) {
                m_actionDependents[ob].erase(node);
            }

            m_dataObservers.erase(node);
            m_actionObservers.erase(node);
        } else {
            for (auto &dep : m_actionDependents[node]) {
                m_actionObservers[dep].erase(node);
            }
            m_actionDependents.erase(node);
        }
        node.reset();
    }

    // Check for cycle dependency between nodes
    bool hasCycle(DataNodePtr source, DataNodePtr target) {
        m_dataDependents[source].insert(target);
        m_dataObservers[target].insert({source, [](bool) {}});

        std::unordered_set<DataNodePtr> visited;
        std::unordered_set<DataNodePtr> recursionStack;
        bool hasCycle = dfs(source, visited, recursionStack);

        m_dataDependents[source].erase(target);
        m_dataObservers[target].erase(source);

        return hasCycle;
    }

    // Depth-first search to detect cycles
    bool dfs(DataNodePtr node, std::unordered_set<DataNodePtr> &visited, std::unordered_set<DataNodePtr> &recursionStack) {
        if (recursionStack.count(node)) return true;
        if (visited.count(node)) return false;

        visited.insert(node);
        recursionStack.insert(node);

        for (auto neighbor : m_dataDependents[node]) {
            if (dfs(neighbor, visited, recursionStack)) {
                return true;
            }
        }

        recursionStack.erase(node);
        return false;
    }

    // Check for repeat dependencies
    template <NodeCC NodeType>
    bool hasRepeatDependencies(std::shared_ptr<NodeType> node, DataNodePtr target) {
        std::unordered_set<DataNodePtr> targetDependencies;
        collectDependencies(target, targetDependencies);

        std::unordered_set<DataNodePtr> visited;
        std::unordered_set<DataNodePtr> dependents;
        if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>) {
            dependents = m_dataDependents[node];
        } else {
            dependents = m_actionDependents[node];
        }

        for (auto dependent : dependents) {
            if (checkDependency(dependent, targetDependencies, visited)) {
                return true;
            }
        }
        return false;
    }

    // Collect all dependencies for a given node
    void collectDependencies(DataNodePtr node, std::unordered_set<DataNodePtr> &dependencies) {
        if (!node || dependencies.count(node)) return;
        dependencies.insert(node);

        for (auto neighbor : m_dataDependents[node]) {
            collectDependencies(neighbor, dependencies);
        }
    }

    // Check if a node is part of a target's dependencies
    bool checkDependency(DataNodePtr node, const std::unordered_set<DataNodePtr> &targetDependencies, std::unordered_set<DataNodePtr> &visited) {
        if (visited.count(node)) return false;
        visited.insert(node);

        if (targetDependencies.count(node)) return true;

        for (auto neighbor : m_dataDependents[node]) {
            if (checkDependency(neighbor, targetDependencies, visited)) return true;
        }

        return false;
    }

    // Maps to store dependencies and observers
    std::unordered_map<DataNodePtr, std::unordered_set<DataNodePtr>> m_dataDependents;
    std::unordered_map<DataNodePtr, std::unordered_map<DataNodePtr, std::function<void(bool)>>> m_dataObservers;
    std::unordered_map<DataNodePtr, std::unordered_map<ActionNodePtr, std::function<void(bool)>>> m_actionObservers;
    std::unordered_map<ActionNodePtr, std::unordered_set<DataNodePtr>> m_actionDependents;
};

// FieldGraph handles field-specific operations
class FieldGraph {
public:
    static FieldGraph &getInstance() {
        static FieldGraph instance;
        return instance;
    }

    // Add a field node to the graph
    void addNode(FieldNodePtr node) {
        m_fieldList.insert(node);
    }

    // Close a field node in the graph
    void closeNode(FieldNodePtr node) {
        m_fieldList.erase(node);
        m_fieldObservers.erase(node.get());
    }

    // Add a field to the graph
    void addField(void *meta, ObserverFieldNode *node) {
        m_fieldMap[meta].insert(node);
    }

    // Delete a field from the graph
    void deleteField(void *meta) {
        m_fieldMap.erase(meta);
    }

    // Set the field associated with a metadata pointer
    void setField(void *meta, DataNodePtr metaPtr) {
        for (auto node : m_fieldMap[meta]) {
            m_fieldObservers[node] = metaPtr;
        }
    }

    // Get the metadata for a field node
    DataNodePtr getMeta(ObserverFieldNode *node) {
        return m_fieldObservers[node];
    }

private:
    FieldGraph() {
    }
    std::unordered_map<void *, std::unordered_set<ObserverFieldNode *>> m_fieldMap;
    std::unordered_map<ObserverFieldNode *, DataNodePtr> m_fieldObservers;
    std::unordered_set<FieldNodePtr> m_fieldList;
};

// Base class for all observer nodes
template <typename Derived>
class ObserverBase : public std::enable_shared_from_this<Derived> {
public:
    using SourceType = DataNode;

    // Constructor with an optional name
    ObserverBase(const std::string &name = "") :
        m_name(name) {
    }

    // Set and get the name of the observer
    void setName(const std::string &name) {
        m_name = name;
    }
    std::string getName() const {
        return m_name;
    }

    // Get a shared pointer to the derived class
    std::shared_ptr<Derived> getShared() {
        return this->shared_from_this();
    }

private:
    std::string m_name;
};

// ObserverDataNode handles data node-specific observers
class ObserverDataNode : public ObserverBase<ObserverDataNode> {
public:
    using SourceType = DataNode;

protected:
    // Notify observers about changes
    void notifyObservers(bool changed = true) {
        DataNodePtr shared_this = getShared();
        for (auto &[observer, fun] : ObserverGraph::getInstance().getDataObserverList(shared_this)) {
            std::invoke(fun, changed);
        }
        for (auto &[observer, fun] : ObserverGraph::getInstance().getActionObserverList(shared_this)) {
            std::invoke(fun, changed);
        }
    }

    // Update observers with the new state
    bool updateObservers(bool &repeat, std::function<void(bool)> &&f, auto &&...args) {
        DataNodePtr shared_this = getShared();
        ObserverGraph::getInstance().resetNode(shared_this);
        if (!(ObserverGraph::getInstance().addObserver(repeat, f, shared_this, args.getShared()) && ...)) {
            ObserverGraph::getInstance().resetNode(shared_this);
            return false;
        }
        return true;
    }
};

// ObserverActionNode handles action node-specific observers
class ObserverActionNode : public ObserverBase<ObserverActionNode> {
public:
    using SourceType = ActionNode;

protected:
    // Update observers for action nodes
    bool updateObservers(bool &repeat, std::function<void(bool)> f, auto &&...args) {
        ActionNodePtr shared_this = getShared();
        ObserverGraph::getInstance().resetNode(shared_this);
        if (!(ObserverGraph::getInstance().addObserver(repeat, f, shared_this, args.getShared()) && ...)) {
            ObserverGraph::getInstance().resetNode(shared_this);
            return false;
        }
        return true;
    }
};

// ObserverFieldNode handles field node-specific observers
class ObserverFieldNode : public ObserverBase<ObserverFieldNode> {
public:
    using SourceType = FieldNode;

protected:
    // Notify observers for field nodes
    void notifyObservers(bool changed = true) {
        auto metaPtr = FieldGraph::getInstance().getMeta(this);
        for (auto &[observer, fun] : ObserverGraph::getInstance().getDataObserverList(metaPtr)) {
            std::invoke(fun, changed);
        }
        for (auto &[observer, fun] : ObserverGraph::getInstance().getActionObserverList(metaPtr)) {
            std::invoke(fun, changed);
        }
    }
};

} // namespace reaction

#endif // REACTION_OBSERVERNODE_H
