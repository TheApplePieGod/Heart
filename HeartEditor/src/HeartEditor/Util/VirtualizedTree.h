#pragma once

#include "Heart/Container/HVector.hpp"
#include "Heart/Core/UUID.h"

namespace HeartEditor
{
    class VirtualizedTree
    {
    public:
        struct Node
        {
            Heart::UUID Id;
            void* UserData;
            bool HasChildren;
        };

    public:
        VirtualizedTree() = default;

        void Rebuild(
            const Heart::HVector<Node>& rootNodes,
            std::function<Heart::HVector<Node>(const Node&)>&& getChildren
        );

        void ExpandNode(Heart::UUID id);
        void UnexpandNode(Heart::UUID id);

        void OnImGuiRender(
            float nodeHeight,
            Heart::UUID selectedNode,
            std::function<void(const Node&)>&& render,
            std::function<void(const Node&)>&& onSelect
        );

    private:
        struct InternalNode
        {
            Node Data;
            u32 IndentCount;
        };

    private:
        void RebuildNode(
            const Heart::HVector<Node>& rootNodes,
            const std::function<Heart::HVector<Node>(const Node&)>& getChildren,
            u32 indentCount
        );

    private:
        Heart::HVector<InternalNode> m_Nodes;
        std::unordered_set<Heart::UUID> m_ExpandedNodes;
        std::unordered_set<Heart::UUID> m_NewExpandedNodes;
    };
}
