#include "hepch.h"
#include "DebugInfo.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "HeartEditor/EditorCamera.h"
#include "Heart/Input/Input.h"
#include "Heart/Core/Timing.h"
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
        double stepMs = EditorApp::Get().GetLastTimestep().StepMilliseconds();
        // ImGui::Text("Render Api: %s", HE_ENUM_TO_STRING(Heart::RenderApi, Heart::Renderer::GetApiType()));
        ImGui::Text("Frametime: %.1fms", stepMs);
        ImGui::Text("Framerate: %d FPS", static_cast<u32>(1000.0 / stepMs));
        ImGui::Unindent();

        auto& viewport = (Widgets::Viewport&)Editor::GetWindow("Viewport");
        ImGui::Text("Viewport 1 Info:");
        ImGui::Indent();
        glm::vec3 cameraPos =  viewport.GetActiveCameraPosition();
        glm::vec3 cameraFor = viewport.GetActiveCamera().GetForwardVector();
        ImGui::Text("Camera Pos: (%.2f, %.2f, %.2f)", cameraPos.x, cameraPos.y, cameraPos.z);
        ImGui::Text("Camera Dir: (%.2f, %.2f, %.2f)", cameraFor.x, cameraFor.y, cameraFor.z);
        ImGui::Text("Camera Rot: (%.2f, %.2f)", viewport.GetActiveCameraRotation().x, viewport.GetActiveCameraRotation().y);
        ImGui::Text("Mouse Pos: (%.1f, %.1f)", Heart::Input::GetScreenMousePos().x, Heart::Input::GetScreenMousePos().y);
        ImGui::Text("VP Mouse: (%.1f, %.1f)", viewport.GetRelativeMousePos().x, viewport.GetRelativeMousePos().y);
        ImGui::Text("VP Hover: %s", viewport.IsHovered() ? "true" : "false");
        ImGui::Unindent();

        ImGui::Text("CPU Timing:");
        ImGui::Indent();
        for (auto& pair : Heart::AggregateTimer::GetTimeMap())
            ImGui::Text("%s: %.1fms", pair.first.Data(), pair.second);
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

        ImGui::Text("Render Statistics:");
        ImGui::Indent();
        /*
        for (auto& pair : Heart::Renderer::GetStatistics())
            ImGui::Text("%s: %lld", pair.first.Data(), pair.second);
        */
        ImGui::Unindent();

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}