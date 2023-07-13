#include "hepch.h"
#include "DevPanel.h"

#include "Heart/Core/Timing.h"

namespace HeartRuntime
{
    void DevPanel::OnImGuiRender(Heart::Scene* scene, Heart::SceneRenderSettings& settings)
    {
        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin("Dev Panel", &m_Open);

        ImGui::Text("SSAO Enable");
        ImGui::SameLine();
        ImGui::Checkbox("##SSAOEnable", &settings.SSAOEnable);
        
        ImGui::Text("Bloom Enable");
        ImGui::SameLine();
        ImGui::Checkbox("##BloomEnable", &settings.BloomEnable);

        ImGui::Text("Cull Enable");
        ImGui::SameLine();
        ImGui::Checkbox("##CullEnable", &settings.CullEnable);

        ImGui::Text("Async Asset Loading");
        ImGui::SameLine();
        ImGui::Checkbox("##AsyncAsset", &settings.AsyncAssetLoading);
        
        ImGui::Text("Render Physics Volumes");
        ImGui::SameLine();
        ImGui::Checkbox("##RenderPhys", &settings.RenderPhysicsVolumes);
        
        ImGui::Separator();

        ImGui::Text("CPU Timing:");
        ImGui::Indent();
        for (auto& pair : Heart::AggregateTimer::GetTimeMap())
            ImGui::Text("%s: %.1fms", pair.first.Data(), pair.second);
        ImGui::Unindent();

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
