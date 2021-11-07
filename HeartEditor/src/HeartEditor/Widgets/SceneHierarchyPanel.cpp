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

        for (auto entity : view)
        {
            auto& nameComponent = view.get<Heart::NameComponent>(entity);

            ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | (m_SelectedEntity.GetHandle() == entity ? ImGuiTreeNodeFlags_Selected : 0);
            bool open = ImGui::TreeNodeEx((void*)(intptr_t)(u32)entity, node_flags, nameComponent.Name.c_str());
            if (ImGui::IsItemClicked())
                m_SelectedEntity = Heart::Entity(activeScene, entity);
            if (open)
            {
                ImGui::Text("Blah blah\nBlah Blah");
                ImGui::TreePop();
            }
        }
    }
}
}