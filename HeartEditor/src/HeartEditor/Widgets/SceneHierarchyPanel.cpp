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

    void SceneHierarchyPanel::OnImGuiRender(Heart::Scene* activeScene)
    {
        // only top level components
        auto view = activeScene->GetRegistry().view<Heart::NameComponent>(entt::exclude<Heart::ParentComponent>);

        ImGui::BeginChild("HierarchyChild");
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.f, 0.f));
        for (auto entity : view)
        {
            RenderEntity(activeScene, entity);
        }
        ImGui::PopStyleVar();
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

    void SceneHierarchyPanel::RenderEntity(Heart::Scene* activeScene, entt::entity entity)
    {
        auto& nameComponent = activeScene->GetRegistry().get<Heart::NameComponent>(entity);
        bool hasChildren = activeScene->GetRegistry().any_of<Heart::ChildComponent>(entity) && activeScene->GetRegistry().get<Heart::ChildComponent>(entity).Children.size() > 0;
        bool open = false;
        std::string nameString = "EntityPopup";

        if (hasChildren)
        {
            // create the tree node if the entity has children
            ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | (m_SelectedEntity.GetHandle() == entity ? ImGuiTreeNodeFlags_Selected : 0);
            open = ImGui::TreeNodeEx((void*)(intptr_t)(u32)entity, node_flags, nameComponent.Name.c_str());
        }
        else
        {
            // otherwise just draw the name
            ImGui::Indent();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.f, 0.f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.5f, 0.f));
            ImGui::Button(nameComponent.Name.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0.f));
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();
            ImGui::Unindent();
        }
        if (ImGui::IsItemClicked())
            m_SelectedEntity = Heart::Entity(activeScene, entity);

        // if dragging, we are going to be looking for a new parent to assign to
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("EntityNode", &entity, sizeof(u64));
            HE_ENGINE_LOG_TRACE(std::to_string(static_cast<u32>(entity)));
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
                activeScene->DestroyEntity({ activeScene, entity });
            if (ImGui::Button("Duplicate Entity"))
            {
                m_SelectedEntity = activeScene->DuplicateEntity({ activeScene, entity });
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // recursively render children components
        if (open)
        {
            if (hasChildren)
            {
                auto& childComp = activeScene->GetRegistry().get<Heart::ChildComponent>(entity);
                for (auto uuid : childComp.Children)
                {
                    RenderEntity(activeScene, activeScene->GetEntityFromUUID(uuid).GetHandle());
                }
            }
            ImGui::TreePop();
        }
    }
}
}