// include/reaction/ObserverNode.h
#ifndef REACTION_OBSERVERNode_H
#define REACTION_OBSERVERNode_H

#include <unordered_set>
#include <unordered_map>

namespace reaction
{
    class ObserverNode;

    class ObserverGraph
    {
    public:
        // 向图中添加一个数据源节点
        void addNode(ObserverNode *node)
        {
            // 如果节点已经存在，则不做任何操作
            m_dependents[node] = std::unordered_set<ObserverNode *>();
            m_observers[node] = std::unordered_set<ObserverNode *>();
        }

        // 添加一个依赖关系
        bool addDependency(ObserverNode *source, ObserverNode *target)
        {
            // 如果source和target相同，不能添加
            if (source == target)
            {
                return false;
            }

            // 检查是否存在环依赖
            if (hasCycle(source, target))
            {
                return false;
            }

            // 添加依赖
            m_dependents[source].insert(target);
            m_observers[target].insert(source);
            return true;
        }

        // 删除节点的所有依赖关系
        void resetNode(ObserverNode *node)
        {
            // 删除该节点的所有依赖
            for (ObserverNode *dep : m_dependents[node])
            {
                m_observers[dep].erase(node);
            }
            m_dependents[node].clear();
        }

        std::unordered_set<ObserverNode *> getObserverList(ObserverNode *node)
        {
            return m_observers[node];
        }

    private:
        // 邻接表（正向依赖关系）
        std::unordered_map<ObserverNode *, std::unordered_set<ObserverNode *>> m_dependents;

        // 邻接表（反向依赖关系，用于检查循环依赖）
        std::unordered_map<ObserverNode *, std::unordered_set<ObserverNode *>> m_observers;

        // 检查是否有环依赖
        bool hasCycle(ObserverNode *source, ObserverNode *target)
        {
            // 临时设置依赖关系
            m_dependents[source].insert(target);
            m_observers[target].insert(source);

            // 使用深度优先搜索（DFS）检测环
            std::unordered_set<ObserverNode *> visited;
            if (dfs(source, visited, target))
            {
                // 如果找到环，则恢复状态
                m_dependents[source].erase(target);
                m_observers[target].erase(source);
                return true;
            }

            // 恢复状态
            m_dependents[source].erase(target);
            m_observers[target].erase(source);
            return false;
        }

        // 使用DFS检测从source到target是否有环
        bool dfs(ObserverNode *node, std::unordered_set<ObserverNode *> &visited, ObserverNode *target)
        {
            if (visited.count(node))
            {
                return node == target;
            }

            visited.insert(node);
            for (ObserverNode *neighbor : m_dependents[node])
            {
                if (dfs(neighbor, visited, target))
                {
                    return true;
                }
            }
            visited.erase(node);
            return false;
        }
    };

    class ObserverNode
    {
    public:
        ObserverNode()
        {
            graphInstance.addNode(this);
        }
        virtual ~ObserverNode() {}

    protected:
        virtual void valueChanged(){}
        void updateObservers(ObserverNode *dataSource, auto &&...args)
        {
            graphInstance.resetNode(this);
            (graphInstance.addDependency(dataSource, &args), ...);
        }
        void notifyObservers()
        {
            for (auto &observer : graphInstance.getObserverList(this))
            {
                observer->valueChanged();
            }
        }
        static ObserverGraph graphInstance;
    };

    ObserverGraph ObserverNode::graphInstance;

} // namespace reaction

#endif // REACTION_OBSERVERNode_H
