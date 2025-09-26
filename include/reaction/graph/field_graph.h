/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/core/types.h"
#include "reaction/graph/observer_graph.h"
#include <unordered_map>

namespace reaction {

/**
 * @brief Manages object field bindings in the reactive graph.
 *
 * Used to track field-depth reactive nodes for specific object IDs.
 */
class FieldGraph {
public:
    /**
     * @brief Get singleton instance.
     * @return FieldGraph& singleton reference.
     */
    [[nodiscard]] static FieldGraph &getInstance() noexcept {
        static FieldGraph instance;
        return instance;
    }

    /**
     * @brief Register a node under an object ID.
     * @param id Object ID.
     * @param node Node pointer.
     */
    void addObj(uint64_t id, const NodePtr &node) noexcept {
        m_fieldMap[id].insert(node);
    }

    /**
     * @brief Remove all fields for an object.
     * @param id Object ID.
     */
    void deleteObj(uint64_t id) noexcept {
        m_fieldMap.erase(id);
    }

    /**
     * @brief Bind all field nodes of an object to an input node.
     *
     * Adds observer edges from objPtr to each field node under the object ID.
     * @param id Object ID.
     * @param objPtr Source node.
     */
    void bindField(uint64_t id, const NodePtr &objPtr) {
        NodeSet nodesToBind;
        if (!m_fieldMap.contains(id)) {
            return;
        }
        nodesToBind = m_fieldMap[id]; // Copy the set

        // Bind nodes
        for (auto &node : nodesToBind) {
            ObserverGraph::getInstance().addObserver(objPtr, node.lock());
        }
    }

private:
    FieldGraph() {}
    std::unordered_map<uint64_t, NodeSet> m_fieldMap; ///< Map from object ID to its field nodes.
};

} // namespace reaction
