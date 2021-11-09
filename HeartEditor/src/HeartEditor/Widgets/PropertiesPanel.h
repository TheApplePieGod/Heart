#pragma once

#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Scene/Components.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
namespace Widgets
{
    class PropertiesPanel
    {
    public:
        PropertiesPanel();

        void OnImGuiRender(Heart::Entity selectedEntity);

    private:
        void RenderXYZSlider(const std::string& name, f32* x, f32* y, f32* z, f32 min, f32 max, f32 step);

        template<typename Component>
        void RenderComponentPopup(const std::string& popupName, Heart::Entity selectedEntity)
        {
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                ImGui::OpenPopup(popupName.c_str());
            if (ImGui::BeginPopup(popupName.c_str()))
            {
                if (ImGui::Button("Remove Component"))
                    selectedEntity.RemoveComponent<Component>();
                ImGui::EndPopup();
            }
        }
    };
}
}