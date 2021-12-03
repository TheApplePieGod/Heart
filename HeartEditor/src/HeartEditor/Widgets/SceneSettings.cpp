#include "hepch.h"
#include "SceneSettings.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Util/ImGuiUtils.h"

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

        Heart::UUID mapId = activeScene.GetEnvironmentMap() ? activeScene.GetEnvironmentMap()->GetMapAsset() : 0;
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

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}