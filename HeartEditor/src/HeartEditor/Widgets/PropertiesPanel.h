#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "HeartEditor/Editor.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Scene/Components.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
namespace Widgets
{
    class PropertiesPanel : public Widget
    {
    public:
        PropertiesPanel(const Heart::HStringView8& name, bool initialOpen)
            : Widget(name, initialOpen)
        {}

        void OnImGuiRender() override;

    private:
        void RenderTransformComponent();
        void RenderMeshComponent();
        void RenderLightComponent();
        void RenderScriptComponent();
        void RenderCameraComponent();
        void RenderCollisionComponent();
        void RenderTextComponent();

        void RenderScriptField(Heart::HStringView fieldName, Heart::ScriptComponent& scriptComp);
        bool RenderCollisionChannels(Heart::HStringView8 id, u32& mask);
        
        // returns true if the component was deleted
        template<typename Component>
        bool RenderComponentPopup(const Heart::HStringView8& popupName, bool canRemove = true)
        {
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                ImGui::OpenPopup(popupName.Data());
            if (ImGui::BeginPopup(popupName.Data()))
            {
                if (!canRemove)
                    ImGui::BeginDisabled();
                if (ImGui::MenuItem("Remove Component"))
                {
                    Editor::GetState().SelectedEntity.RemoveComponent<Component>();
                    ImGui::EndPopup();
                    return true;
                }
                if (!canRemove)
                    ImGui::EndDisabled();
                ImGui::EndPopup();
            }
            return false;
        }
    
    private:
        ImGuiTextFilter m_MeshTextFilter;
        ImGuiTextFilter m_MaterialTextFilter;
        ImGuiTextFilter m_ScriptTextFilter;
        ImGuiTextFilter m_FontTextFilter;
    };
}
}
