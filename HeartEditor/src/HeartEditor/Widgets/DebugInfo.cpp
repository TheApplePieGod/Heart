#include "hepch.h"
#include "DebugInfo.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "HeartEditor/EditorCamera.h"
#include "Heart/Input/Input.h"
#include "Heart/Core/Timing.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/RenderPlugin.h"
#include "Flourish/Api/Context.h"
#include "imgui/imgui.h"

#include "HeartEditor/Widgets/Viewport.h"

namespace HeartEditor
{
namespace Widgets
{
    void DebugInfo::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);

        ImGui::Text("Basic Info:");
        ImGui::Indent();
        double stepMs = EditorApp::Get().GetAveragedTimestep().StepMilliseconds();
        // ImGui::Text("Render Api: %s", HE_ENUM_TO_STRING(Heart::RenderApi, Heart::Renderer::GetApiType()));
        ImGui::Text("Frametime: %.1fms", stepMs);
        ImGui::Text("Framerate: %d FPS", static_cast<u32>(1000.0 / stepMs));
        ImGui::Unindent();

        auto& viewport = (Widgets::Viewport&)Editor::GetWindow("Viewport");
        ImGui::Text("Viewport Info:");
        ImGui::Indent();
        glm::vec3 cameraPos =  viewport.GetActiveCameraPosition();
        glm::vec3 cameraFor = viewport.GetActiveCamera().GetForwardVector();
        ImGui::Text("Camera Pos: (%.2f, %.2f, %.2f)", cameraPos.x, cameraPos.y, cameraPos.z);
        ImGui::Text("Camera Dir: (%.2f, %.2f, %.2f)", cameraFor.x, cameraFor.y, cameraFor.z);
        ImGui::Text("Camera Rot: (%.2f, %.2f)", viewport.GetActiveCameraRotation().x, viewport.GetActiveCameraRotation().y);
        ImGui::Text("Mouse Pos: (%.1f, %.1f)", Heart::Input::GetMousePosition().x, Heart::Input::GetMousePosition().y);
        ImGui::Text("VP Mouse: (%.1f, %.1f)", viewport.GetRelativeMousePos().x, viewport.GetRelativeMousePos().y);
        ImGui::Text("VP Hover: %s", viewport.IsHovered() ? "true" : "false");
        ImGui::Unindent();

        ImGui::Text("CPU Timing:");
        ImGui::Indent();
        for (auto& pair : Heart::AggregateTimer::GetTimeMap())
            ImGui::Text("%s: %.1fms", pair.first.Data(), Heart::AggregateTimer::GetAggregateTime(pair.first));
        ImGui::Unindent();

        ImGui::Text("GPU Timing:");
        ImGui::Indent();
        /*
        ImGui::Text("Frustum cull: %.2fms", viewport.GetSceneRenderer().GetCullPipeline().GetPerformanceTimestamp());
        ImGui::Text("Opaque Pass: %.2fms", viewport.GetSceneRenderer().GetMainFramebuffer().GetSubpassPerformanceTimestamp(2));
        ImGui::Text("Translucent Pass: %.2fms", viewport.GetSceneRenderer().GetMainFramebuffer().GetSubpassPerformanceTimestamp(3));
        double bloomTiming = 0.0;
        for (auto& bufs : viewport.GetSceneRenderer().GetBloomFramebuffers())
        {
            bloomTiming += bufs[0]->GetPerformanceTimestamp();
            bloomTiming += bufs[1]->GetPerformanceTimestamp();
        }
        ImGui::Text("Bloom Pass: %.2fms", bloomTiming);
        */
        ImGui::Unindent();

        ImGui::Text("GPU Memory:");
        ImGui::Indent();
        Flourish::MemoryStatistics memoryStats = Flourish::Context::ComputeMemoryStatistics();
        ImGui::Text("Allocations: %d", memoryStats.AllocationCount);
        ImGui::Text("Allocation Mem: %.1f MB", (float)memoryStats.AllocationTotalSize / 1e6);
        ImGui::Text("Blocks: %d", memoryStats.BlockCount);
        ImGui::Text("Block Mem: %.1f MB", (float)memoryStats.BlockTotalSize / 1e6);
        ImGui::Text("Total Avail: %.1f MB", (float)memoryStats.TotalAvailable / 1e6);
        ImGui::Unindent();

        ImGui::Text("Render Statistics:");
        ImGui::Indent();
        ImGui::Text("Plugins:");
        for (const auto& pair : viewport.GetSceneRenderer().GetPlugins())
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
        /*
        for (auto& pair : Heart::Renderer::GetStatistics())
            ImGui::Text("%s: %lld", pair.first.Data(), pair.second);
        */
        ImGui::Unindent();
        
        auto& activeScene = Editor::GetActiveScene();
        ImGui::Text("Scene info:");
        ImGui::Indent();
        ImGui::Text("Entity count: %d", (int)activeScene.GetRegistry().alive());
        ImGui::Unindent();
        
        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}
