#include "hepch.h"
#include "DevPanel.h"

#include "HeartRuntime/RuntimeApp.h"
#include "Flourish/Api/Context.h"
#include "Heart/Core/Timing.h"

namespace HeartRuntime
{
    void DevPanel::OnImGuiRender(
        Viewport* viewport,
        Heart::Scene* scene,
        Heart::SceneRenderSettings& settings
    )
    {
        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin("Dev Panel", &m_Open);

        double stepMs = RuntimeApp::Get().GetAveragedTimestep().StepMilliseconds();
        ImGui::Text("Frametime: %.1fms", stepMs);
        ImGui::Text("Framerate: %d FPS", static_cast<u32>(1000.0 / stepMs));

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
            ImGui::Text("%s: %.1fms", pair.first.Data(), Heart::AggregateTimer::GetAggregateTime(pair.first));
        ImGui::Unindent();

        ImGui::Text("Render Statistics:");
        ImGui::Indent();
        ImGui::Text("Plugins:");
        for (const auto& pair : viewport->GetSceneRenderer()->GetPlugins())
        {
            if (pair.second->GetStats().empty())
                continue;

            ImGui::Indent();
            ImGui::Text("%s:", pair.first.Data());
            for (const auto& stat : pair.second->GetStats())
            {
                if (stat.second.Type == Heart::RenderPlugin::StatType::None)
                    continue;
                ImGui::Indent();
                ImGui::Text("%s:", stat.first.Data());
                ImGui::SameLine();
                switch (stat.second.Type)
                {
                    default:
                    {} break;
                    case Heart::RenderPlugin::StatType::Int:
                    { ImGui::Text("%d", stat.second.Data.Int); } break;
                    case Heart::RenderPlugin::StatType::Float:
                    { ImGui::Text("%2.f", stat.second.Data.Float); } break;
                    case Heart::RenderPlugin::StatType::Bool:
                    { ImGui::Text("%s", stat.second.Data.Bool ? "true" : "false"); } break;
                    case Heart::RenderPlugin::StatType::TimeMS:
                    { ImGui::Text("%1.fms", stat.second.Data.Float); } break;
                }
                ImGui::Unindent();
            }
            ImGui::Unindent();
        }
        ImGui::Unindent();

        ImGui::Text("GPU Memory:");
        ImGui::Indent();
        Flourish::MemoryStatistics memoryStats = Flourish::Context::ComputeMemoryStatistics();
        ImGui::Text("Allocations: %u", memoryStats.AllocationCount);
        ImGui::Text("Allocation Mem: %.1f MB", (float)memoryStats.AllocationTotalSize / 1e6);
        ImGui::Text("Blocks: %u", memoryStats.BlockCount);
        ImGui::Text("Block Mem: %.1f MB", (float)memoryStats.BlockTotalSize / 1e6);
        ImGui::Text("Total Avail: %.1f MB", (float)memoryStats.TotalAvailable / 1e6);
        ImGui::Unindent();

        //HE_LOG_WARN("Memory stats: {}", memoryStats.ToString());

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
