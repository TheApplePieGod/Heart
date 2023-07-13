#include "hepch.h"
#include "RenderSettings.h"

#include "HeartEditor/Editor.h"

namespace HeartEditor
{
namespace Widgets
{
    void RenderSettings::OnImGuiRenderPostSceneUpdate()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);

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

        ImGui::Text("Bloom Strength");
        ImGui::SameLine();
        ImGui::DragFloat("##BloomStrength", &Editor::GetState().RenderSettings.BloomStrength, 0.01f, 0.f, 2.f);

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
        
        ImGui::Text("Render Physics Volumes");
        ImGui::SameLine();
        ImGui::Checkbox("##RenderPhys", &Editor::GetState().RenderSettings.RenderPhysicsVolumes);

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}
