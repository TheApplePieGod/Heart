#include "hepch.h"
#include "SceneSettings.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Util/ImGuiUtils.h"
#include "glm/trigonometric.hpp"

namespace HeartEditor
{
namespace Widgets
{
    void SceneSettings::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.c_str(), &m_Open);

        auto& activeScene = Editor::GetActiveScene();

        Heart::UUID mapId = 0;
        if (activeScene.GetEnvironmentMap())
            mapId = activeScene.GetEnvironmentMap()->GetMapAsset();
        ImGui::Text("Environment map:");
        ImGui::SameLine();
        Heart::ImGuiUtils::AssetPicker(
            Heart::Asset::Type::Texture,
            mapId,
            "NULL",
            "EnvMapSelect",
            m_EnvMapTextFilter,
            nullptr,
            [&](Heart::UUID selected)
            {
                activeScene.SetEnvironmentMap(selected);
            }
        );

        float angle = glm::degrees(std::atan2(activeScene.GetSunAngle().y, activeScene.GetSunAngle().x));
        ImGui::Text("Sun Angle");
        ImGui::SameLine();
        if (ImGui::DragFloat("##SunAngle", &angle, 1.f, -179.f, 179.f))
            activeScene.SetSunAngle({ cos(glm::radians(angle)), sin(glm::radians(angle)), 0.f });

        glm::vec3 color = activeScene.GetSunColor();
        ImGui::Text("Sun Color");
        ImGui::SameLine();
        if (ImGui::ColorEdit3("##SunColor", (float*)&color))
            activeScene.SetSunColor(color);

        float intensity = activeScene.GetSunIntensity();
        ImGui::Text("Sun Intensity");
        ImGui::SameLine();
        if (ImGui::DragFloat("##SunIntensity", &intensity, 0.5f, 0.f, 100.f))
            activeScene.SetSunIntensity(intensity);

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}