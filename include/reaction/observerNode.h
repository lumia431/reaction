#include "reaction/utility.h"

namespace reaction {

class ObserverGraph {
public:
    static ObserverGraph &getInstance() {
        static ObserverGraph instance;
        return instance;
    }

    void addNode(NodePtr node);

    bool addObserver(NodePtr source, NodePtr target) {
        if (source == target) {
            Log::error("Cannot observe self, node = {}.", m_nameList[source]);
            return false;
        }
        if (hasCycle(source, target)) {
            Log::error("Cycle dependency detected, node = {}. Cycle dependent = {}", m_nameList[source], m_nameList[target]);
            return false;
        }

        hasRepeatDependencies(source, target);

        m_dependentList.at(source).insert(target);
        m_observerList.at(target).get().insert({source});
        return true;
    }

    void resetNode(NodePtr node) {
        cleanupDependencies(node);
        m_dependentList.at(node).clear();
    }

    void closeNode(NodePtr node) {
        if (!node) return;
        NodeSet closedNodes;
        cascadeCloseDependents(node, closedNodes);
    }

    void setName(NodePtr node, const std::string &name) {
        m_nameList.insert({node, name});
    }

    std::string getName(NodePtr node) {
        if (m_nameList.contains(node)) {
            return m_nameList[node];
        } else {
            return "";
        }
    }

private:
    ObserverGraph() {}

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

    void cascadeCloseDependents(NodePtr node, NodeSet &closedNodes) {
        if (!node || closedNodes.contains(node)) return;
        closedNodes.insert(node);
        auto observerLists = m_observerList.at(node).get();

        for (auto &ob : observerLists) {
            cascadeCloseDependents(ob.lock(), closedNodes);
        }

        closeNodeInternal(node);
    }

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

    void hasRepeatDependencies(NodePtr source, NodePtr target) {

        NodeSet dependencies;
        collectDependencies(target, dependencies);

        NodeSet visited;
        for (auto &dependent : m_dependentList.at(source)) {
            checkDependency(source, dependent.lock(), dependencies, visited);
        }
    }

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

    void collectDependencies(NodePtr node, NodeSet &dependencies) {
        NodeMap targetDependencies;
        collectDependencies(node, targetDependencies);

        for (auto &[dependent, count] : targetDependencies) {
            if (count == 1) {
                dependencies.insert(dependent);
            }
        }
    }

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

    void collectObservers(NodePtr node, NodeSet &observers) {
        NodeMap targetObservers;
        collectDependencies(node, targetObservers);

        for (auto &[observer, count] : targetObservers) {
            if (count == 1) {
                observers.insert(observer);
            }
        }
    }

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

    std::unordered_map<NodePtr, NodeSetRef> m_observerList;
    std::unordered_map<NodePtr, NodeSet> m_dependentList;
    std::unordered_map<NodePtr, NodeMapRef> m_repeatList;
    std::unordered_map<NodePtr, std::string> m_nameList;
};

class ObserverNode : public std::enable_shared_from_this<ObserverNode> {
public:
    virtual ~ObserverNode() = default;

    virtual void valueChanged() {
        this->notify();
    }

    bool updateObservers(auto &&...args) {
        auto shared_this = shared_from_this();
        ObserverGraph::getInstance().resetNode(shared_this);
        if (!(ObserverGraph::getInstance().addObserver(shared_this, args) && ...)) {
            ObserverGraph::getInstance().resetNode(shared_this);
            return false;
        }
        return true;
    }

    void addOneObserver(NodePtr node) {
        ObserverGraph::getInstance().addObserver(shared_from_this(), node);
    }

    void notify() {
        NodeSet repeatList;
        notify(repeatList);
    }

    void notify(NodeSet &repeatList) {
        Log::info("node {} trigger.", ObserverGraph::getInstance().getName(shared_from_this()));

        for (auto &[repeat, _] : m_repeats) {
            repeatList.insert(repeat);
        }
        for (auto &observer : m_observers) {
            if (repeatList.find(observer) == repeatList.end()) {
                observer.lock()->valueChanged();
            }
        }

        if (!repeatList.empty()) {
            for (auto &[repeat, _] : m_repeats) {
                repeat.lock()->valueChanged();
                repeatList.erase(repeat);
            }
        }
    }

private:
    NodeSet m_observers;
    NodeMap m_repeats;
    friend class ObserverGraph;
};

class FieldGraph {
public:
    static FieldGraph &getInstance() {
        static FieldGraph instance;
        return instance;
    }
    void addObj(uint64_t id, NodePtr node) {
        m_fieldMap[id].insert(node);
    }

    void deleteObj(uint64_t id) {
        m_fieldMap.erase(id);
    }

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
    std::unordered_map<uint64_t, NodeSet> m_fieldMap;
};

inline void ObserverGraph::addNode(NodePtr node) {
    m_observerList.insert({node, std::ref(node->m_observers)});
    m_dependentList[node] = NodeSet{};
    m_repeatList.insert({node, std::ref(node->m_repeats)});
}
} // namespace reaction