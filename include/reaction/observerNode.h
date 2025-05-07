#include "reaction/concept.h"
#include "reaction/utility.h"
#include <vector>
#include <unordered_set>

namespace reaction
{
    class ObserverNode : public std::enable_shared_from_this<ObserverNode>
    {
    public:
        virtual void valueChanged() 
        {
            this->notify();
        }

        void addObserver(ObserverNode *observer)
        {
            m_observers.emplace_back(observer);
        }

        template <typename... Args>
        void updateObservers(Args &&...args)
        {
            (..., args.getPtr()->addObserver(this));
        }

        void notify()
        {
            for (auto &observer : m_observers)
            {
                observer->valueChanged();
            }
        }

    private:
        std::vector<ObserverNode *> m_observers;
    };

    class ObserverGraph
    {
    public:
        static ObserverGraph &getInstance()
        {
            static ObserverGraph instance;
            return instance;
        }
        void addNode(NodePtr node)
        {
            m_nodes.insert(node);
        }

        void removeNode(NodePtr node)
        {
            m_nodes.erase(node);
        }

    private:
        ObserverGraph()
        {
        }
        std::unordered_set<NodePtr> m_nodes;
    };

    class FieldGraph
    {
    public:
        static FieldGraph &getInstance()
        {
            static FieldGraph instance;
            return instance;
        }
        void addObj(uint64_t id, NodePtr node)
        {
            m_fieldMap[id].insert(node);
        }

        void deleteObj(uint64_t id)
        {
            m_fieldMap.erase(id);
        }

        void bindField(uint64_t id, NodePtr objPtr)
        {
            if (!m_fieldMap.count(id))
            {
                return;
            }
            for (auto &node : m_fieldMap[id])
            {
                node->addObserver(objPtr.get());
            }
        }

    private:
        FieldGraph()
        {
        }
        std::unordered_map<uint64_t, std::unordered_set<NodePtr>> m_fieldMap;
    };
}