#include "hepch.h"
#include "SceneHierarchyPanel.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "Heart/Container/HString.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Entity.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace HeartEditor
{
namespace Widgets
{
    //typedef std::pair<Heart::HStringView, entt::entity> TreeNodeData;
    void SceneHierarchyPanel::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);

        // Only top level components
        auto& activeScene = Editor::GetActiveScene();
        auto view = activeScene.GetRegistry().view<Heart::IdComponent, Heart::NameComponent>(entt::exclude<Heart::ParentComponent>);

        // Order by name
        m_RootNodes.Clear();
        for (auto handle : view)
        {
            Heart::Entity entity(&activeScene, handle);
            m_RootNodes.Add({ entity.GetUUID(), (void*)handle, entity.HasChildren() });
        }
        std::sort(
            m_RootNodes.Begin(),
            m_RootNodes.End(),
            [&view](const auto& a, const auto& b)
            {
                return view.get<Heart::NameComponent>((entt::entity)(uintptr_t)a.UserData).Name < view.get<Heart::NameComponent>((entt::entity)(uintptr_t)b.UserData).Name;
            }
        );

        m_Tree.Rebuild(
            m_RootNodes,
            [](const auto& node)
            {
                Heart::Entity entity(&Editor::GetActiveScene(), (entt::entity)(uintptr_t)node.UserData);
                Heart::HVector<VirtualizedTree::Node> nodes;

                auto& children = entity.GetChildren();
                nodes.Reserve(children.Count());
                for (auto uuid : children)
                {
                    auto child = entity.GetScene()->GetEntityFromUUID(uuid);
                    nodes.Add({ child.GetUUID(), (void*)child.GetHandle(), child.HasChildren() });
                }

                return std::move(nodes);
            }
        );

        ImGui::BeginChild("HierarchyChild");
        const float nodeHeight = 18.f;
        m_Tree.OnImGuiRender(
            nodeHeight,
            Editor::GetState().SelectedEntity.IsValid() ? Editor::GetState().SelectedEntity.GetUUID() : Heart::UUID(0),
            [&](const VirtualizedTree::Node& node)
            {
                Heart::Entity entity = { &activeScene, (entt::entity)(uintptr_t)node.UserData };
                Heart::HStringView name = entity.GetName();

                // if dragging, we are going to be looking for a new parent to assign to
                if (ImGui::BeginDragDropSource())
                {
                    ImGui::SetDragDropPayload("EntityNode", &node.UserData, sizeof(entt::entity));
                    ImGui::Text("%s", name.DataUTF8());
                    ImGui::EndDragDropSource();
                }

                // if dropped on, set this entity as the new parent and expand
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityNode"))
                    {
                        entt::entity payloadData = *(const entt::entity*)payload->Data;
                        activeScene.AssignRelationship(entity, { &activeScene, payloadData });

                        m_Tree.ExpandNode(node.Id);
                    }
                    ImGui::EndDragDropTarget();
                }

                // right click menu
                bool deleted = false;
                ImGui::PushID((void*)(uintptr_t)node.Id);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Create Child Entity"))
                    {
                        auto newEntity = Editor::GetActiveScene().CreateEntity("New Entity");
                        activeScene.AssignRelationship(entity, newEntity);
                        m_Tree.ExpandNode(node.Id);
                    }
                    if (ImGui::MenuItem("Remove Entity"))
                    {
                        activeScene.DestroyEntity(entity, true);
                        Editor::GetState().SelectedEntity = Heart::Entity();
                        deleted = true;
                    }
                    if (ImGui::MenuItem("Duplicate Entity"))
                    {
                        Editor::GetState().SelectedEntity = activeScene.DuplicateEntity(entity, true, true);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                ImGui::PopID();

                ImGui::BeginHorizontal("helem", { -1.f, nodeHeight }, 0.5f);
                ImGui::Text("%s", deleted ? "<Deleted>" : name.DataUTF8());
                ImGui::EndHorizontal();
            },
            [&activeScene](const VirtualizedTree::Node& node)
            {
                Editor::GetState().SelectedEntity = Heart::Entity(&activeScene, (entt::entity)(uintptr_t)node.UserData);
            }
        );
        ImGui::EndChild();

        // top level drag drop for parenting to root
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityNode"))
            {
                u32 payloadData = *(const u32*)payload->Data;
                Editor::GetActiveScene().UnparentEntity({ &Editor::GetActiveScene(), payloadData });
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginPopupContextItem("SceneHierarchyPopup", ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem("Create Entity"))
                Editor::GetState().SelectedEntity = Editor::GetActiveScene().CreateEntity("New Entity");

            ImGui::EndPopup();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}
