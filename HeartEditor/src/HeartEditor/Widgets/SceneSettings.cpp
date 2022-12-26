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
        ImGui::Begin(m_Name.Data(), &m_Open);

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

        ImGui::Separator();

        ImGui::Text("Draw Grid");
        ImGui::SameLine();
        ImGui::Checkbox("##DrawGrid", &Editor::GetState().RenderSettings.DrawGrid);

        ImGui::Text("SSAO Enable");
        ImGui::SameLine();
        ImGui::Checkbox("##SSAOEnable", &Editor::GetState().RenderSettings.SSAOEnable);
        
        ImGui::Text("SSAO Radius");
        ImGui::SameLine();
        ImGui::DragFloat("##SSAORad", &Editor::GetState().RenderSettings.SSAORadius, 0.05f, 0.f, 5.f);
        
        ImGui::Text("SSAO Bias");
        ImGui::SameLine();
        ImGui::DragFloat("##SSAOBias", &Editor::GetState().RenderSettings.SSAOBias, 0.005f, 0.f, 1.f);
        
        ImGui::Text("SSAO Kernel Size");
        ImGui::SameLine();
        ImGui::DragInt("##SSAOKern", &Editor::GetState().RenderSettings.SSAOKernelSize, 1.f, 0, 64);
         
        ImGui::Text("Bloom Enable");
        ImGui::SameLine();
        ImGui::Checkbox("##BloomEnable", &Editor::GetState().RenderSettings.BloomEnable);

        ImGui::Text("Bloom Threshold");
        ImGui::SameLine();
        ImGui::DragFloat("##BloomThresh", &Editor::GetState().RenderSettings.BloomThreshold, 0.1f, 0.f, 5.f);

        ImGui::Text("Bloom Knee");
        ImGui::SameLine();
        ImGui::DragFloat("##BBKne", &Editor::GetState().RenderSettings.BloomKnee, 0.01f, 0.f, 1.f);

        ImGui::Text("Bloom Sample Scale");
        ImGui::SameLine();
        ImGui::DragFloat("##BBSS", &Editor::GetState().RenderSettings.BloomSampleScale, 0.1f, 0.f, 10.f);

        ImGui::Text("Cull Enable");
        ImGui::SameLine();
        ImGui::Checkbox("##CullEnable", &Editor::GetState().RenderSettings.CullEnable);

        ImGui::Text("Async Asset Loading");
        ImGui::SameLine();
        ImGui::Checkbox("##AsyncAsset", &Editor::GetState().RenderSettings.AsyncAssetLoading);

        ImGui::Text("Update Entity Ids Texture");
        ImGui::SameLine();
        ImGui::Checkbox("##EntityIds", &Editor::GetState().RenderSettings.CopyEntityIdsTextureToCPU);

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}
