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
        ImGui::Begin(m_Name.Data(), &m_Open);

        auto& activeScene = Editor::GetActiveScene();

        Heart::UUID mapId = 0;
        if (activeScene.GetEnvironmentMap())
            mapId = activeScene.GetEnvironmentMap()->GetMapAsset();
        ImGui::Text("Environment map:");
        ImGui::SameLine();
        m_AssetPicker.OnImGuiRender(
            mapId,
            Heart::Asset::Type::Texture,
            "None",
            [&]()
            {
                if (!mapId)
                    return;

                if (ImGui::MenuItem("Clear"))
                    activeScene.SetEnvironmentMap(0);
            },
            [&](Heart::UUID selected)
            {
                activeScene.SetEnvironmentMap(selected);
            }
        );
        
        if (ImGui::CollapsingHeader("Physics Settings"))
        {
            ImGui::Indent();
            
            glm::vec3 grav = activeScene.GetPhysicsWorld().GetGravity();
            if (Heart::ImGuiUtils::XYZSlider(
                "Gravity  ",
                &grav.x,
                &grav.y,
                &grav.z,
                -100.f,
                100.f,
                0.25f)
            )
                activeScene.GetPhysicsWorld().SetGravity(grav);
            
            ImGui::Unindent();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}
