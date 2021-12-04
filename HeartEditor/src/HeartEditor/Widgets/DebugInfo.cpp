#include "hepch.h"
#include "DebugInfo.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
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
        ImGui::Begin(m_Name.c_str(), &m_Open);

        ImGui::Text("Basic Info:");
        ImGui::Indent();
        double stepMs = EditorApp::Get().GetLastTimestep().StepMilliseconds();
        ImGui::Text("Render Api: %s", HE_ENUM_TO_STRING(Heart::RenderApi, Heart::Renderer::GetApiType()));
        ImGui::Text("Frametime: %.1fms", stepMs);
        ImGui::Text("Framerate: %d FPS", static_cast<u32>(1000.0 / stepMs));
        ImGui::Unindent();

        auto& viewport = (Widgets::Viewport&)Editor::GetWindow("Viewport");
        ImGui::Text("Viewport 1 Info:");
        ImGui::Indent();
        glm::vec3 cameraPos =  viewport.GetCamera().GetPosition();
        glm::vec3 cameraFor = viewport.GetCamera().GetForwardVector();
        ImGui::Text("Camera Pos: (%.2f, %.2f, %.2f)", cameraPos.x, cameraPos.y, cameraPos.z);
        ImGui::Text("Camera Dir: (%.2f, %.2f, %.2f)", cameraFor.x, cameraFor.y, cameraFor.z);
        ImGui::Text("Camera Rot: (%.2f, %.2f)", viewport.GetCamera().GetXRotation(), viewport.GetCamera().GetYRotation());
        ImGui::Text("Mouse Pos: (%.1f, %.1f)", Heart::Input::GetScreenMousePos().x, Heart::Input::GetScreenMousePos().y);
        ImGui::Text("VP Mouse: (%.1f, %.1f)", viewport.GetRelativeMousePos().x, viewport.GetRelativeMousePos().y);
        ImGui::Text("VP Hover: %s", viewport.IsHovered() ? "true" : "false");
        ImGui::Unindent();

        ImGui::Text("Timing:");
        ImGui::Indent();
        for (auto& pair : Heart::AggregateTimer::GetTimeMap())
            ImGui::Text("%s: %.1fms", pair.first.c_str(), pair.second);
        ImGui::Unindent();

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}