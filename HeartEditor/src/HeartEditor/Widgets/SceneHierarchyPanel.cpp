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
        auto view = activeScene->GetRegistry().view<Heart::NameComponent>();

        auto nameString = std::string("EntityPopup");
        for (auto entity : view)
        {
            auto& nameComponent = view.get<Heart::NameComponent>(entity);

            ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | (m_SelectedEntity.GetHandle() == entity ? ImGuiTreeNodeFlags_Selected : 0);
            bool open = ImGui::TreeNodeEx((void*)(intptr_t)(u32)entity, node_flags, nameComponent.Name.c_str());
            if (ImGui::IsItemClicked())
                m_SelectedEntity = Heart::Entity(activeScene, entity);
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
            if (open)
            {
                ImGui::Text("Blah blah\nBlah Blah");
                ImGui::TreePop();
            }
        }
    }
}
}