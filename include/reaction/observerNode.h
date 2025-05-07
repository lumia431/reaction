#include "reaction/concept.h"
#include <memory>
#include <vector>
#include <unordered_set>

namespace reaction
{
    class ObserverNode : public std::enable_shared_from_this<ObserverNode>
    {
    public:
        virtual void valueChanged() {}

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

    using NpdePtr = std::shared_ptr<ObserverNode>;
    class ObserverGraph
    {
    public:
        static ObserverGraph &getInstance()
        {
            static ObserverGraph instance;
            return instance;
        }
        void addNode(NpdePtr node)
        {
            m_nodes.insert(node);
        }

        void removeNode(NpdePtr node)
        {
            m_nodes.erase(node);
        }

    private:
        ObserverGraph()
        {
        }
        std::unordered_set<NpdePtr> m_nodes;
    };
}