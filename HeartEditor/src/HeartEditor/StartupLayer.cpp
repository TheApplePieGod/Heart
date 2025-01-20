#include "hepch.h"
#include "StartupLayer.h"

#include "HeartEditor/EditorApp.h"
#include "HeartEditor/Editor.h"
#include "HeartEditor/Project.h"
#include "Heart/Core/Window.h"
#include "Heart/Events/KeyEvents.h"
#include "Heart/Events/ButtonEvents.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "imgui/imgui_internal.h"

namespace HeartEditor
{
    void StartupLayer::OnAttach()
    {
        HE_PROFILE_FUNCTION();

        Heart::WindowCreateInfo startupWindow;
        startupWindow.Title = "Heart Editor";
        startupWindow.Width = 960;
        startupWindow.Height = 540;
        EditorApp::Get().OpenWindow(startupWindow);

        SubscribeToEmitter(&EditorApp::Get().GetWindow());

        HE_LOG_INFO("Startup layer attached");
    }

    void StartupLayer::OnDetach()
    {
        UnsubscribeFromEmitter(&EditorApp::Get().GetWindow());

        HE_LOG_INFO("Startup layer detached");
    }

    void StartupLayer::OnUpdate(Heart::Timestep ts)
    {
        HE_PROFILE_FUNCTION();

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::Begin("Main Window", nullptr, windowFlags);

        m_ProjectPicker.OnImGuiRender();

        ImGui::End();

        ImGui::PopStyleVar(2);

        // Process project selection once imgui rendering has finished
        auto& selectedProject = m_ProjectPicker.GetSelectedProject();
        if (!selectedProject.Path.IsEmpty())
        {
            ((EditorApp&)EditorApp::Get()).StartEditor(selectedProject.Name);
            if (selectedProject.New)
                Project::CreateAndLoad(selectedProject.Path, selectedProject.Name);
            else
                Project::LoadFromPath(selectedProject.Path);
        }
    }

    void StartupLayer::OnEvent(Heart::Event& event)
    {
        event.Map<Heart::KeyPressedEvent>(HE_BIND_EVENT_FN(StartupLayer::KeyPressedEvent));
        event.Map<Heart::ButtonPressedEvent>(HE_BIND_EVENT_FN(StartupLayer::ButtonPressedEvent));
        event.Map<Heart::ButtonReleasedEvent>(HE_BIND_EVENT_FN(StartupLayer::ButtonReleasedEvent));
    }

    bool StartupLayer::KeyPressedEvent(Heart::KeyPressedEvent& event)
    {
        return true;
    }

    bool StartupLayer::ButtonPressedEvent(Heart::ButtonPressedEvent& event)
    {
        return true;
    }

    bool StartupLayer::ButtonReleasedEvent(Heart::ButtonReleasedEvent& event)
    {
        return false;
    }
}
