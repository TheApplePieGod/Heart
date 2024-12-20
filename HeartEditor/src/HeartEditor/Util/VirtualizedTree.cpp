#include "hepch.h"
#include "VirtualizedTree.h"

#include "HeartEditor/Editor.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Util/ImGuiUtils.h"
#include "imgui/imgui.h"
#include "imgui_internal.h"

namespace HeartEditor
{
    void VirtualizedTree::Rebuild(
        const Heart::HVector<Node>& rootNodes,
        std::function<Heart::HVector<Node>(const Node&)>&& getChildren
    ) {
        HE_PROFILE_FUNCTION();

        m_Nodes.Clear();
        m_NewExpandedNodes.clear();

        RebuildNode(rootNodes, std::move(getChildren), 0);

        // Ensure expanded node list doesn't contain stale nodes
        m_ExpandedNodes = m_NewExpandedNodes;
    }

    void VirtualizedTree::RebuildNode(
        const Heart::HVector<Node>& rootNodes,
        const std::function<Heart::HVector<Node>(const Node&)>& getChildren,
        u32 indentCount
    ) {
        for (u32 i = 0; i < rootNodes.Count(); i++)
        {
            const auto& node = rootNodes[i];        

            // Add parent to the node list
            m_Nodes.Add({ node, indentCount });

            // Add children
            bool nest = m_ExpandedNodes.count(node.Id) > 0;
            if (nest && node.HasChildren)
            {
                m_NewExpandedNodes.insert(node.Id);
                auto children = getChildren(node);
                RebuildNode(children, getChildren, indentCount + 1);
            }
        }
    }

    void VirtualizedTree::ExpandNode(Heart::UUID id)
    {
        m_ExpandedNodes.insert(id);
    }

    void VirtualizedTree::UnexpandNode(Heart::UUID id)
    {
        m_ExpandedNodes.erase(id);
    }

    void VirtualizedTree::OnImGuiRender(
        float nodeHeight,
        Heart::UUID selectedNode,
        std::function<void(const Node&)>&& render,
        std::function<void(const Node&)>&& onSelect
    ) {
        HE_PROFILE_FUNCTION();
        
        ImGuiListClipper clipper; // For virtualizing the list
        clipper.Begin(m_Nodes.Count());
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            {
                if (i >= m_Nodes.Count()) break;

                auto& node = m_Nodes[i];

                const float padding = 4.f;
                float indent = ImGui::GetStyle().IndentSpacing * node.IndentCount;
                bool open = m_ExpandedNodes.count(node.Data.Id) > 0;
                bool selected = selectedNode == node.Data.Id;
                
                if (indent > 0.f)
                    ImGui::Indent(indent);

                ImGui::PushID((void*)(uintptr_t)node.Data.Id);

                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
                bool clicked = ImGui::InvisibleButton("##elem", { -1.f, nodeHeight + 2.f * padding });
                bool hovered = ImGui::IsItemHovered();
                bool doubleClicked = hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
                ImVec2 buttonMin = ImGui::GetItemRectMin();
                ImVec2 buttonMax = ImGui::GetItemRectMax();
                ImGui::PopStyleColor();

                // Render background
                ImU32 bgColor = selected ? IM_COL32(60, 60, 60, 255) : IM_COL32(0, 0, 0, 0);
                if (hovered)
                {
                    float factor = selected ? 75 : 45;
                    bgColor = IM_COL32(factor, factor, factor, 255);
                }
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(buttonMin, buttonMax, bgColor);

                float arrowSize = 18.f;
                if (node.Data.HasChildren)
                {
                    ImVec2 arrowCenter = { buttonMin.x + padding, buttonMin.y + nodeHeight * 0.5f };
                    ImGui::RenderArrow(
                        ImGui::GetWindowDrawList(),
                        arrowCenter,
                        IM_COL32(255, 255, 255, 255),
                        open ? ImGuiDir_Down : ImGuiDir_Right,
                        0.75f
                    );
                }

                ImGui::SetCursorScreenPos({ buttonMin.x + padding + arrowSize, buttonMin.y + padding });
                render(node.Data);
                ImGui::SetCursorScreenPos({ buttonMin.x, buttonMax.y });

                ImGui::PopID();

                bool clickedArrow = ImGui::GetMousePos().x < buttonMin.x + padding + arrowSize;
                if (node.Data.HasChildren && ((clicked && clickedArrow) || doubleClicked))
                {
                    if (open)
                        UnexpandNode(node.Data.Id);
                    else
                        ExpandNode(node.Data.Id);
                }
                else if (clicked && !clickedArrow)
                    onSelect(node.Data);

                if (indent > 0.f)
                    ImGui::Unindent(indent);
            }
        }
    }
}
