/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/react.h"

namespace reaction {

template <typename SrcType, IsInvaStra IS = KeepStra, IsTrigMode TM = ChangeTrig>
using Var = React<VarExpr, SrcType, IS, TM>;

template <typename SrcType, IsInvaStra IS = KeepStra, IsTrigMode TM = ChangeTrig>
using Calc = React<CalcExpr, SrcType, IS, TM>;

class FieldBase {
public:
    template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = KeepStra, typename T>
    auto field(T &&t) {
        auto ptr = std::make_shared<ReactImpl<VarExpr, std::decay_t<T>, IS, TM>>(std::forward<T>(t));
        ObserverGraph::getInstance().addNode(ptr->shared_from_this());
        FieldGraph::getInstance().addObj(m_id, ptr->shared_from_this());
        return React(ptr);
    }

    uint64_t getId() {
        return m_id;
    }

private:
    UniqueID m_id;
};

template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = KeepStra, typename SrcType>
auto constVar(SrcType &&t) {
    auto ptr = std::make_shared<ReactImpl<VarExpr, const std::decay_t<SrcType>, IS, TM>>(std::forward<SrcType>(t));
    ObserverGraph::getInstance().addNode(ptr);
    return React(ptr);
}

template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = KeepStra, typename SrcType>
auto var(SrcType &&t) {
    auto ptr = std::make_shared<ReactImpl<VarExpr, std::decay_t<SrcType>, IS, TM>>(std::forward<SrcType>(t));
    ObserverGraph::getInstance().addNode(ptr);
    if constexpr (HasField<SrcType>) {
        FieldGraph::getInstance().bindField(t.getId(), ptr->shared_from_this());
    }
    return React(ptr);
}

template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = KeepStra, typename OpExpr>
auto expr(OpExpr &&opExpr) {
    auto ptr = std::make_shared<ReactImpl<CalcExpr, std::decay_t<OpExpr>, IS, TM>>(std::forward<OpExpr>(opExpr));
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set();
    return React{ptr};
}

template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = KeepStra, typename Fun, typename... Args>
auto calc(Fun &&fun, Args &&...args) {
    auto ptr = std::make_shared<ReactImpl<CalcExpr, ReturnType<Fun, Args...>, IS, TM>>();
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set(std::forward<Fun>(fun), std::forward<Args>(args)...);
    return React(ptr);
}

template <IsTrigMode TM = ChangeTrig, IsInvaStra IS = KeepStra, typename Fun, typename... Args>
auto action(Fun &&fun, Args &&...args) {
    return calc<TM, IS>(std::forward<Fun>(fun), std::forward<Args>(args)...);
}

template <typename T>
auto make(T &&t) {
    if constexpr (IsBinaryOpExpr<std::decay_t<T>>) {
        return expr(std::forward<T>(t));
    } else if constexpr (InvocableType<T>) {
        return calc(std::forward<T>(t));
    } else {
        return var(std::forward<T>(t));
    }
}

template <typename Fun, typename... Args>
auto make(Fun &&fun, Args &&...args) {
    return calc(std::forward<Fun>(fun), std::forward<Args>(args)...);
}

} // namespace reaction