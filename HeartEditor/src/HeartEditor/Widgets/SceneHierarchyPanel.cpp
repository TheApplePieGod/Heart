#include "hepch.h"
#include "SceneHierarchyPanel.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "Heart/Container/HString.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Entity.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace HeartEditor
{
namespace Widgets
{
    void SceneHierarchyPanel::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);

        // Only top level components
        auto view = Editor::GetActiveScene().GetRegistry().view<Heart::NameComponent>(entt::exclude<Heart::ParentComponent>);

        // Order by name
        std::multimap<Heart::HString, entt::entity> nameMap;
        for (auto entity : view)
            nameMap.insert({ view.get<Heart::NameComponent>(entity).Name, entity });

        ImGui::BeginChild("HierarchyChild");
        for (auto& pair : nameMap)
            if (RenderEntity(pair.second))
                break;
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

    bool SceneHierarchyPanel::RenderEntity(entt::entity entity)
    {
        Heart::Scene& activeScene = Editor::GetActiveScene();

        auto& nameComponent = activeScene.GetRegistry().get<Heart::NameComponent>(entity);
        bool hasChildren = activeScene.GetRegistry().any_of<Heart::ChildrenComponent>(entity) && activeScene.GetRegistry().get<Heart::ChildrenComponent>(entity).Children.Count() > 0;
        bool open = false;
        bool justDestroyed = false;
        Heart::HStringView8 nameString = "EntityPopup";

        // create the tree node
        ImGuiTreeNodeFlags node_flags = (hasChildren ? 0 : ImGuiTreeNodeFlags_Leaf) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | (Editor::GetState().SelectedEntity.GetHandle() == entity ? ImGuiTreeNodeFlags_Selected : 0);
        open = ImGui::TreeNodeEx((void*)(intptr_t)(u32)entity, node_flags, nameComponent.Name.DataUTF8());
        if (ImGui::IsItemClicked())
            Editor::GetState().SelectedEntity = Heart::Entity(&activeScene, entity);

        // if dragging, we are going to be looking for a new parent to assign to
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("EntityNode", &entity, sizeof(u64));
            ImGui::Text(nameComponent.Name.DataUTF8());
            ImGui::EndDragDropSource();
        }

        // if dropped on, set this entity as the new parent
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityNode"))
            {
                u32 payloadData = *(const u32*)payload->Data;
                activeScene.AssignRelationship({ &activeScene, entity }, { &activeScene, payloadData });
            }
            ImGui::EndDragDropTarget();
        }

        // right click menu
        if (ImGui::BeginPopupContextItem((nameString + std::to_string(static_cast<u32>(entity))).Data()))
        {
            if (ImGui::MenuItem("Create Child Entity"))
            {
                auto newEntity = Editor::GetActiveScene().CreateEntity("New Entity");
                activeScene.AssignRelationship({ &activeScene, entity }, newEntity);
            }
            if (ImGui::MenuItem("Remove Entity"))
            {
                activeScene.DestroyEntity({ &activeScene, entity });
                Editor::GetState().SelectedEntity = Heart::Entity();
                justDestroyed = true;
            }
            if (ImGui::MenuItem("Duplicate Entity"))
            {
                Editor::GetState().SelectedEntity = activeScene.DuplicateEntity({ &activeScene, entity }, true, true);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (justDestroyed)
        {
            ImGui::TreePop();
            return true;
        }

        // recursively render children components
        if (open)
        {
            if (hasChildren)
            {
                // Order by name
                std::multimap<Heart::HString, entt::entity> nameMap;
                auto& childComp = activeScene.GetRegistry().get<Heart::ChildrenComponent>(entity);
                for (auto uuid : childComp.Children)
                {
                    auto entity = activeScene.GetEntityFromUUID(uuid);
                    nameMap.insert({ entity.GetName(), entity.GetHandle() });
                }
                for (auto& pair : nameMap)
                    RenderEntity(pair.second);
            }
            ImGui::TreePop();
        }

        return false;
    }
}
}