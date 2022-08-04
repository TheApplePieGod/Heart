#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "HeartEditor/Editor.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Scene/Components.h"
#include "imgui/imgui.h"

namespace Heart
{
    class HString;
}

namespace HeartEditor
{
namespace Widgets
{
    class PropertiesPanel : public Widget
    {
    public:
        PropertiesPanel(const std::string& name, bool initialOpen)
            : Widget(name, initialOpen)
        {}

        void OnImGuiRender() override;

    private:
        void RenderTransformComponent();
        void RenderMeshComponent();
        void RenderLightComponent();
        void RenderScriptComponent();
        void RenderCameraComponent();

        void RenderXYZSlider(const std::string& name, f32* x, f32* y, f32* z, f32 min, f32 max, f32 step);
        void RenderScriptField(const Heart::HString& fieldName, Heart::ScriptComponent& scriptComp);

        // returns true if the component was deleted
        template<typename Component>
        bool RenderComponentPopup(const std::string& popupName, bool canRemove = true)
        {
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                ImGui::OpenPopup(popupName.c_str());
            if (ImGui::BeginPopup(popupName.c_str()))
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
    };
}
}