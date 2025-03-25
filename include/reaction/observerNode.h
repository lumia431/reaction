#ifndef REACTION_OBSERVERNODE_H
#define REACTION_OBSERVERNODE_H

#include "reaction/log.h"
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <functional>

namespace reaction
{
    struct DataNode
    {
    };
    struct ActionNode
    {
    };

    class ObserverDataNode;
    class ObserverActionNode;

    using DataNodePtr = std::shared_ptr<ObserverDataNode>;
    using ActionNodePtr = std::shared_ptr<ObserverActionNode>;

    static std::unordered_map<DataNodePtr, std::function<void(bool)>> g_dataNotifyMap;
    static std::unordered_map<ActionNodePtr, std::function<void(bool)>> g_actionNotifyMap;

    class FieldGraph
    {
    public:
        static FieldGraph &getInstance()
        {
            static FieldGraph instance;
            return instance;
        }

        void addObserver(DataNodePtr meta, DataNodePtr field)
        {
            m_fieldDependents[meta].insert(field);
        }

        std::unordered_set<DataNodePtr> getObserverList(DataNodePtr meta)
        {
            return m_fieldDependents[meta];
        }

        void deleteNode(DataNodePtr meta)
        {
            m_fieldDependents.erase(meta);
        }

    private:
        FieldGraph() {}
        std::unordered_map<DataNodePtr, std::unordered_set<DataNodePtr>> m_fieldDependents;
    };

    class ObserverGraph
    {
    public:
        static ObserverGraph &getInstance()
        {
            static ObserverGraph instance;
            return instance;
        }

        template <typename NodeType>
        void addNode(std::shared_ptr<NodeType> node)
        {
            if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>)
            {
                m_dataDependents[node] = std::unordered_set<DataNodePtr>();
                m_dataObservers[node] = std::unordered_set<DataNodePtr>();
                m_actionObservers[node] = std::unordered_set<ActionNodePtr>();
            }
            else
            {
                m_actionDependents[node] = std::unordered_set<DataNodePtr>();
            }
        }

        template <typename NodeType, typename TargetType>
        bool addObserver(bool &repeat, std::shared_ptr<NodeType> source, std::shared_ptr<TargetType> target)
        {
            if constexpr (std::is_same_v<NodeType, TargetType>)
            {
                if (source == target)
                {
                    Log::error("cannot observe self, node = {}.", source->getName());
                    return false;
                }
            }

            if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>)
            {
                if (hasCycle(source, target))
                {
                    Log::error("detect cycle dependency, node = {}. cycle dependent = {}", source->getName(), target->getName());
                    return false;
                }
            }

            if (!repeat && hasRepeatDependencies(source, target))
            {
                Log::info("detect repeat dependency, node = {}. repeat dependent = {}", source->getName(), target->getName());
                repeat = true;
            }

            if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>)
            {
                m_dataDependents[source].insert(target);
                m_dataObservers[target].insert(source);
            }
            else
            {
                m_actionDependents[source].insert(target);
                m_actionObservers[target].insert(source);
            }
            return true;
        }

        template <typename NodeType>
        void resetNode(std::shared_ptr<NodeType> node)
        {
            if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>)
            {
                for (auto dep : m_dataDependents[node])
                {
                    m_dataObservers[dep].erase(node);
                }
                m_dataDependents[node].clear();
            }
            else
            {
                for (auto dep : m_actionDependents[node])
                {
                    m_actionObservers[dep].erase(node);
                }
                m_actionDependents[node].clear();
            }
        }

        template <typename NodeType>
        void closeNode(std::shared_ptr<NodeType> node)
        {
            if (!node)
                return;

            std::unordered_set<std::shared_ptr<void>> closedNodes;
            cascadeCloseDependents(node, closedNodes);
        }

        template <typename NodeType>
        void deleteNode(std::shared_ptr<NodeType> node)
        {
            if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>)
            {
                m_dataDependents.erase(node);
                m_dataObservers.erase(node);
                m_actionObservers.erase(node);
                g_dataNotifyMap.erase(node);
            }
            else
            {
                m_actionDependents.erase(node);
                g_actionNotifyMap.erase(node);
            }
        }

        std::unordered_set<DataNodePtr> getDataObserverList(DataNodePtr node)
        {
            return m_dataObservers[node];
        }

        std::unordered_set<ActionNodePtr> getActionObserverList(DataNodePtr node)
        {
            return m_actionObservers[node];
        }

    private:
        ObserverGraph() {}

        // 级联关闭所有依赖目标节点的节点
        template <typename NodeType>
        void cascadeCloseDependents(std::shared_ptr<NodeType> node,
                                    std::unordered_set<std::shared_ptr<void>> &closedNodes)
        {
            if (!node || closedNodes.count(node))
                return;
            closedNodes.insert(node);

            // 只关闭依赖此节点的观察者
            if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>)
            {
                auto dataNode = std::static_pointer_cast<ObserverDataNode>(node);

                // 复制观察者集合避免迭代器失效
                auto dataObservers = m_dataObservers[dataNode];
                auto actionObservers = m_actionObservers[dataNode];

                for (auto &ob : dataObservers)
                {
                    cascadeCloseDependents(ob, closedNodes);
                }
                for (auto &ob : actionObservers)
                {
                    cascadeCloseDependents(ob, closedNodes);
                }
            }

            // 关闭当前节点
            closeNodeInternal(node);
        }

        // 内部关闭实现（不级联）
        template <typename NodeType>
        void closeNodeInternal(std::shared_ptr<NodeType> node)
        {
            if (!node)
                return;

            if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>)
            {
                // 清理依赖关系
                for (auto &dep : m_dataDependents[node])
                {
                    m_dataObservers[dep].erase(node);
                }
                m_dataDependents.erase(node);

                // 清理观察关系
                for (auto &ob : m_dataObservers[node])
                {
                    m_dataDependents[ob].erase(node);
                }
                for (auto &ob : m_actionObservers[node])
                {
                    m_actionDependents[ob].erase(node);
                }

                m_dataObservers.erase(node);
                m_actionObservers.erase(node);
                g_dataNotifyMap.erase(node);
            }
            else
            {
                for (auto &dep : m_actionDependents[node])
                {
                    m_actionObservers[dep].erase(node);
                }
                m_actionDependents.erase(node);
                g_actionNotifyMap.erase(node);
            }

            node.reset();
        }

        bool hasCycle(DataNodePtr source, DataNodePtr target)
        {
            // 临时插入边
            m_dataDependents[source].insert(target);
            m_dataObservers[target].insert(source);

            // 检测循环依赖
            std::unordered_set<DataNodePtr> visited;
            std::unordered_set<DataNodePtr> recursionStack; // 用于检测循环

            bool hasCycle = dfs(source, visited, recursionStack);

            // 移除临时插入的边
            m_dataDependents[source].erase(target);
            m_dataObservers[target].erase(source);

            return hasCycle;
        }

        bool dfs(DataNodePtr node, std::unordered_set<DataNodePtr> &visited, std::unordered_set<DataNodePtr> &recursionStack)
        {
            if (recursionStack.count(node))
            {
                // 如果当前节点已经在递归栈中，说明存在循环依赖
                return true;
            }

            if (visited.count(node))
            {
                // 如果当前节点已经访问过，但不在递归栈中，说明没有循环依赖
                return false;
            }

            // 标记当前节点为已访问，并加入递归栈
            visited.insert(node);
            recursionStack.insert(node);

            // 递归检查所有邻居节点
            for (auto neighbor : m_dataDependents[node])
            {
                if (dfs(neighbor, visited, recursionStack))
                {
                    return true;
                }
            }

            // 从递归栈中移除当前节点
            recursionStack.erase(node);
            return false;
        }

        template <typename NodeType>
        bool hasRepeatDependencies(std::shared_ptr<NodeType> node, DataNodePtr target)
        {
            // 1. 收集 target 的所有依赖（包括 target 自身）
            std::unordered_set<DataNodePtr> targetDependencies;
            collectDependencies(target, targetDependencies);

            // 2. 检查 source 的依赖链是否包含 target 或其依赖
            std::unordered_set<DataNodePtr> visited;
            std::unordered_set<DataNodePtr> dependents;
            if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>)
            {
                dependents = m_dataDependents[node];
            }
            else
            {
                dependents = m_actionDependents[node];
            }
            for (auto dependent : dependents)
            {
                if (checkDependency(dependent, targetDependencies, visited))
                {
                    return true;
                }
            }
            return false;
        }

        // 递归收集 target 的所有依赖（包括自身）
        void collectDependencies(DataNodePtr node, std::unordered_set<DataNodePtr> &dependencies)
        {
            if (!node || dependencies.count(node))
                return;                // 避免重复处理
            dependencies.insert(node); // 包括自身

            for (auto neighbor : m_dataDependents[node])
            {
                collectDependencies(neighbor, dependencies);
            }
        }

        // 检查 source 的依赖链是否包含 targetDependencies 中的任意节点
        bool checkDependency(DataNodePtr node,
                             const std::unordered_set<DataNodePtr> &targetDependencies,
                             std::unordered_set<DataNodePtr> &visited)
        {
            if (visited.count(node))
            {
                return false;
            }
            visited.insert(node);

            // 如果当前节点是 target 或其依赖节点，返回 true
            if (targetDependencies.count(node))
            {
                return true;
            }

            // 递归检查所有依赖
            for (auto neighbor : m_dataDependents[node])
            {
                if (checkDependency(neighbor, targetDependencies, visited))
                {
                    return true;
                }
            }

            return false;
        }

        std::unordered_map<DataNodePtr, std::unordered_set<DataNodePtr>> m_dataDependents;
        std::unordered_map<DataNodePtr, std::unordered_set<DataNodePtr>> m_dataObservers;
        std::unordered_map<DataNodePtr, std::unordered_set<ActionNodePtr>> m_actionObservers;
        std::unordered_map<ActionNodePtr, std::unordered_set<DataNodePtr>> m_actionDependents;
    };

    template <typename Derived>
    class ObserverBase : public std::enable_shared_from_this<Derived>
    {
    public:
        using SourceType = DataNode;
        ~ObserverBase()
        {
        }

        ObserverBase(const std::string &name = "") : m_name(name) {}
        void setName(const std::string &name) { m_name = name; }
        std::string getName() { return m_name; }

    protected:
        std::shared_ptr<Derived> getShared() { return this->shared_from_this(); }

    private:
        std::string m_name;
    };

    class ObserverDataNode : public ObserverBase<ObserverDataNode>
    {
        friend class ObserverActionNode;

    public:
        using SourceType = DataNode;

    protected:
        void notifyObservers(bool changed = true)
        {
            DataNodePtr shared_this = getShared();
            for (auto &observer : ObserverGraph::getInstance().getDataObserverList(shared_this))
            {
                if (g_dataNotifyMap.count(observer))
                {
                    std::invoke(g_dataNotifyMap[observer], changed);
                }
            }
            for (auto &observer : ObserverGraph::getInstance().getActionObserverList(shared_this))
            {
                if (g_actionNotifyMap.count(observer))
                {
                    std::invoke(g_actionNotifyMap[observer], changed);
                }
            }
        }

        bool updateObservers(bool &repeat, std::function<void(bool)> f, auto &&...args)
        {
            DataNodePtr shared_this = getShared();
            ObserverGraph::getInstance().resetNode(shared_this);
            if (!(ObserverGraph::getInstance().addObserver(repeat, shared_this, args->getShared()) && ...))
            {
                ObserverGraph::getInstance().resetNode(shared_this);
                return false;
            }
            g_dataNotifyMap[shared_this] = f;
            return true;
        }
    };

    class ObserverActionNode : public ObserverBase<ObserverActionNode>
    {
    public:
        using SourceType = ActionNode;

    protected:
        bool updateObservers(bool &repeat, std::function<void(bool)> f, auto &&...args)
        {
            ActionNodePtr shared_this = getShared();
            ObserverGraph::getInstance().resetNode(shared_this);
            if (!(ObserverGraph::getInstance().addObserver(repeat, shared_this, args->getShared()) && ...))
            {
                ObserverGraph::getInstance().resetNode(shared_this);
                return false;
            }
            g_actionNotifyMap[shared_this] = f;
            return true;
        }
    };
} // namespace reaction

#endif // REACTION_OBSERVERNODE_H