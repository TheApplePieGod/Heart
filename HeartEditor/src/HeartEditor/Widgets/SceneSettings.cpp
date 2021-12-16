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

        ImGui::Separator();

        ImGui::Text("Draw Grid");
        ImGui::SameLine();
        ImGui::Checkbox("##DrawGrid", &Editor::GetState().RenderSettings.DrawGrid);

        ImGui::Text("Bloom Enable");
        ImGui::SameLine();
        ImGui::Checkbox("##BloomEnable", &Editor::GetState().RenderSettings.BloomEnable);

        ImGui::Text("Bloom Threshold");
        ImGui::SameLine();
        ImGui::DragFloat("##BloomThresh", &Editor::GetState().RenderSettings.BloomThreshold, 0.1f, 0.f, 5.f);

        ImGui::Text("Bloom Blur Scale");
        ImGui::SameLine();
        ImGui::DragFloat("##BBScale", &Editor::GetState().RenderSettings.BloomBlurScale, 0.1f, 0.f, 50.f);

        ImGui::Text("Bloom Blur Strength");
        ImGui::SameLine();
        ImGui::DragFloat("##BBStren", &Editor::GetState().RenderSettings.BloomBlurStrength, 0.1f, 0.f, 50.f);

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}