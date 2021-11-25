#include "htpch.h"
#include "SceneHierarchyPanel.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Entity.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace HeartEditor
{
namespace Widgets
{
    SceneHierarchyPanel::SceneHierarchyPanel()
    {

    }

    void SceneHierarchyPanel::OnImGuiRender(Heart::Scene* activeScene, Heart::Entity& selectedEntity)
    {
        HE_PROFILE_FUNCTION();
        
        // only top level components
        auto view = activeScene->GetRegistry().view<Heart::NameComponent>(entt::exclude<Heart::ParentComponent>);

        ImGui::BeginChild("HierarchyChild");
        for (auto entity : view)
        {
            RenderEntity(activeScene, entity, selectedEntity);
        }
        ImGui::EndChild();

        // top level drag drop for parenting to root
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityNode"))
            {
                u32 payloadData = *(const u32*)payload->Data;
                activeScene->UnparentEntity({ activeScene, payloadData });
            }
            ImGui::EndDragDropTarget();
        }
    }

    void SceneHierarchyPanel::RenderEntity(Heart::Scene* activeScene, entt::entity entity, Heart::Entity& selectedEntity)
    {
        auto& nameComponent = activeScene->GetRegistry().get<Heart::NameComponent>(entity);
        bool hasChildren = activeScene->GetRegistry().any_of<Heart::ChildComponent>(entity) && activeScene->GetRegistry().get<Heart::ChildComponent>(entity).Children.size() > 0;
        bool open = false;
        bool justDestroyed = false;
        std::string nameString = "EntityPopup";

        // create the tree node
        ImGuiTreeNodeFlags node_flags = (hasChildren ? 0 : ImGuiTreeNodeFlags_Leaf) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | (selectedEntity.GetHandle() == entity ? ImGuiTreeNodeFlags_Selected : 0);
        open = ImGui::TreeNodeEx((void*)(intptr_t)(u32)entity, node_flags, nameComponent.Name.c_str());
        if (ImGui::IsItemClicked())
            selectedEntity = Heart::Entity(activeScene, entity);

        // if dragging, we are going to be looking for a new parent to assign to
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("EntityNode", &entity, sizeof(u64));
            ImGui::EndDragDropSource();
        }

        // if dropped on, set this entity as the new parent
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityNode"))
            {
                u32 payloadData = *(const u32*)payload->Data;
                activeScene->AssignRelationship({ activeScene, entity }, { activeScene, payloadData });
            }
            ImGui::EndDragDropTarget();
        }

        // right click menu
        if (ImGui::BeginPopupContextItem((nameString + std::to_string(static_cast<u32>(entity))).c_str()))
        {
            if (ImGui::Button("Remove Entity"))
            {
                activeScene->DestroyEntity({ activeScene, entity });
                selectedEntity = Heart::Entity();
                justDestroyed = true;
            }
            if (ImGui::Button("Duplicate Entity"))
            {
                selectedEntity = activeScene->DuplicateEntity({ activeScene, entity }, true, true);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // recursively render children components
        if (open)
        {
            if (hasChildren && !justDestroyed)
            {
                auto& childComp = activeScene->GetRegistry().get<Heart::ChildComponent>(entity);
                for (auto uuid : childComp.Children)
                {
                    RenderEntity(activeScene, activeScene->GetEntityFromUUID(uuid).GetHandle(), selectedEntity);
                }
            }
            ImGui::TreePop();
        }
    }
}
}