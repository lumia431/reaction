#ifndef REACTION_INVALIDSTRATEGY_H
#define REACTION_INVALIDSTRATEGY_H

#include "reaction/observerNode.h"

namespace reaction
{
    template <typename TriggerPolicy, typename InvalidStrategy, typename Type, typename... Args>
    class DataSource;

    struct DirectFailureStrategy
    {
    protected:
        template <typename NodeType>
        void handleInvalid(std::shared_ptr<NodeType> node)
        {
            ObserverGraph::getInstance().closeNode(node);
        }
    };

    struct KeepCalculateStrategy
    {
    protected:
        template <typename NodeType>
        void handleInvalid(std::shared_ptr<NodeType> node) {}
    };

    struct UseLastValidValueStrategy
    {
    protected:
        template <typename DataSourcePtr>
        void handleInvalid(DataSourcePtr ds)
        {
            auto val = ds->get();
            ds->set([=](){ return val;});
        }
    };
} // namespace reaction

#endif // REACTION_INVALIDSTRATEGY_H
