#pragma once

#include "reaction/react.h"
#include <map>
#include <vector>

namespace reaction {

template <typename SrcType, IsInvaStra IS = CloseStra, IsTrigMode TM = ChangeTrig>
using Field = React<ReactImpl<TM, IS, std::decay_t<SrcType>>>;

class FieldBase {
public:
    template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = CloseStra, typename T>
    auto field(T &&t) {
        auto ptr = std::make_shared<ReactImpl<TM, IS, std::decay_t<T>>>(std::forward<T>(t));
        ObserverGraph::getInstance().addNode(ptr->shared_from_this());
        FieldGraph::getInstance().addObj(m_id, ptr->shared_from_this());
        return React(ptr);
    }

    u_int64_t getId() {
        return m_id;
    }

private:
    UniqueID m_id;
};

template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = CloseStra, typename SrcType>
auto constVar(SrcType &&t) {
    auto ptr = std::make_shared<ReactImpl<TM, IS, const std::decay_t<SrcType>>>(std::forward<SrcType>(t));
    ObserverGraph::getInstance().addNode(ptr);
    return React(ptr);
}

template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = CloseStra, typename SrcType>
auto var(SrcType &&t) {
    auto ptr = std::make_shared<ReactImpl<TM, IS, std::decay_t<SrcType>>>(std::forward<SrcType>(t));
    ObserverGraph::getInstance().addNode(ptr);
    if constexpr (HasField<SrcType>) {
        FieldGraph::getInstance().bindField(t.getId(), ptr->shared_from_this());
    }
    return React(ptr);
}

template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = CloseStra, typename OpExpr>
auto expr(OpExpr &&opExpr) {
    auto ptr = std::make_shared<ReactImpl<TM, IS, std::decay_t<OpExpr>>>(std::forward<OpExpr>(opExpr));
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set();
    return React{ptr};
}

template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = CloseStra, typename Fun, typename... Args>
auto calc(Fun &&fun, Args &&...args) {
    auto ptr = std::make_shared<ReactImpl<TM, IS, std::decay_t<Fun>, std::decay_t<Args>...>>();
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set(std::forward<Fun>(fun), std::forward<Args>(args)...);
    return React(ptr);
}

template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = CloseStra, typename Fun, typename... Args>
auto action(Fun &&fun, Args &&...args) {
    return calc<TM, IS>(std::forward<Fun>(fun), std::forward<Args>(args)...);
}

struct ReactAsKey {};
struct ReactAsValue {};

template <typename Expr, typename Type, template <typename...> typename Container = std::vector, typename... Args>
using ReactContain = Container<std::unique_ptr<ReactBase<Expr, Type>>, Args...>;

template <typename Expr, typename Key, typename Value, template <typename...> typename Container = std::map,
    typename Pos = ReactAsValue, typename... Args>
using ReactMap = std::conditional_t<IsReactAsKey<Pos>,
    Container<std::unique_ptr<ReactBase<Expr, Key>>, Value, Args...>,
    Container<Key, std::unique_ptr<ReactBase<Expr, Value>>, Args...>>;

template <typename React>
auto unique(React &&react) {
    return std::make_unique<std::decay_t<React>>(std::forward<React>(react));
}

} // namespace reaction