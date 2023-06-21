#include "hepch.h"
#include "EditorLayer.h"

#include "HeartEditor/Widgets/Viewport.h"
#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "HeartEditor/Project.h"
#include "Heart/Core/Window.h"
#include "Heart/Task/TaskManager.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Plugins/EntityIds.h"
#include "Flourish/Api/Texture.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Input/Input.h"
#include "Heart/Events/KeyboardEvents.h"
#include "Heart/Events/MouseEvents.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "imgui/imgui_internal.h"

namespace HeartEditor
{
    void EditorLayer::OnAttach()
    {
        SubscribeToEmitter(&EditorApp::Get().GetWindow());

        Editor::Initialize();

        // Crappy default project solution until we get some sort of settings file
        /*
        if (std::filesystem::exists("D:/Projects/Heart/HeartProjects/DemoGame/DemoGame.heproj"))
            Project::LoadFromPath("D:/Projects/Heart/HeartProjects/DemoGame/DemoGame.heproj");
            */
        if (std::filesystem::exists("../../../HeartProjects/TestProject/TestProject.heproj"))
            Project::LoadFromPath("../../../HeartProjects/TestProject/TestProject.heproj");
        else
            Editor::CreateWindows();

        Heart::ScriptingEngine::SetScriptInputEnabled(false);

        HE_LOG_INFO("Editor attached");
    }

    void EditorLayer::OnDetach()
    {
        UnsubscribeFromEmitter(&EditorApp::Get().GetWindow());

        Editor::DestroyWindows();
        Editor::Shutdown();

        HE_LOG_INFO("Editor detached");
    }

    void EditorLayer::OnUpdate(Heart::Timestep ts)
    {
        HE_PROFILE_FUNCTION();

        Editor::GetSceneUpdateTask().Wait();
        ((Widgets::Viewport&)Editor::GetWindow("Viewport")).GetSceneRendererUpdateTask().Wait();

        Editor::GetRenderScene().CopyFromScene(&Editor::GetActiveScene());

        auto& viewport = (Widgets::Viewport&)Editor::GetWindow("Viewport");
        viewport.UpdateCamera();

        /*
         * ImGui render
         */

        ImGuizmo::BeginFrame();

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::Begin("Main Window", nullptr, windowFlags);
        
        m_MenuBar.OnImGuiRender();
        m_Toolbar.OnImGuiRender();

        ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.f, 0.f), 0);

        Editor::RenderWindows();

        // Start updating after because we still need scene data to correctly render gui stuff
        Editor::SetSceneUpdateTask(Heart::TaskManager::Schedule([ts]()
        {
            if (Editor::GetSceneState() == SceneState::Playing)
                Editor::GetActiveScene().OnUpdateRuntime(ts);
        }, Heart::Task::Priority::High, "Editor SceneUpdate"));

        // TODO: replace this system with a more streamlined 'render group' system
        Editor::RenderWindowsPostSceneUpdate();

        ImGui::End();

        ImGui::PopStyleVar(2);

        // Update camera after we render the viewport so that it doesn't look like we're lagging behind
        // this only applies when doing deferred scene rendering
        //auto& viewport = (Widgets::Viewport&)Editor::GetWindow("Viewport");
        //viewport.UpdateCamera();
    }

    void EditorLayer::OnEvent(Heart::Event& event)
    {
        event.Map<Heart::KeyPressedEvent>(HE_BIND_EVENT_FN(EditorLayer::KeyPressedEvent));
        event.Map<Heart::MouseButtonPressedEvent>(HE_BIND_EVENT_FN(EditorLayer::MouseButtonPressedEvent));
        event.Map<Heart::MouseButtonReleasedEvent>(HE_BIND_EVENT_FN(EditorLayer::MouseButtonReleasedEvent));
    }

    bool EditorLayer::KeyPressedEvent(Heart::KeyPressedEvent& event)
    {
        if (Editor::GetSceneState() == SceneState::Playing &&
            event.GetKeyCode() == Heart::KeyCode::Escape)
        {
            auto& viewport = (Widgets::Viewport&)Editor::GetWindow("Viewport");
            if (!viewport.IsFocused())
                Editor::StopScene();
        }
        else if (event.GetKeyCode() == Heart::KeyCode::F11)
            EditorApp::Get().GetWindow().ToggleFullscreen();
        else if (event.GetKeyCode() == Heart::KeyCode::S && Heart::Input::IsKeyPressed(Heart::KeyCode::LeftCtrl))
        {
            Project::GetActiveProject()->SaveToDisk();
            Editor::SaveScene();
        }
        else if (
            event.GetKeyCode() == Heart::KeyCode::B &&
            Heart::Input::IsKeyPressed(Heart::KeyCode::LeftCtrl) &&
            Editor::GetSceneState() != SceneState::Playing)
        {
            if (Project::GetActiveProject()->BuildScripts(true))
                Project::GetActiveProject()->LoadScriptsPlugin();
        }
            
        return true;
    }

    bool EditorLayer::MouseButtonPressedEvent(Heart::MouseButtonPressedEvent& event)
    {
        // screen picking
        auto& viewport = (Widgets::Viewport&)Editor::GetWindow("Viewport");
        if (event.GetMouseCode() == Heart::MouseCode::LeftButton &&
            (!ImGuizmo::IsOver() || !Editor::GetState().SelectedEntity.IsValid()) &&
            !viewport.IsFocused()
            && viewport.IsHovered())
        {
            glm::vec2 mousePos = viewport.GetRelativeMousePos();
            glm::vec2 size = viewport.GetSize();

            // TODO: generic way in scenerenderer to retrieve these common plugins or constant name or something
            auto plugin = viewport.GetSceneRenderer().GetPlugin<Heart::RenderPlugins::EntityIds>("EntityIds");
            
            // the image is scaled down in the viewport, so we need to adjust what pixel we are sampling from
            u32 width = plugin->GetTexture()->GetWidth();
            u32 height = plugin->GetTexture()->GetHeight();
            u32 sampleX = static_cast<u32>(mousePos.x / size.x * width);
            u32 sampleY = static_cast<u32>(mousePos.y / size.y * height);

            float entityId;
            plugin->GetBuffer()->ReadBytes(&entityId, 4, (sampleX + sampleY * width) * 4);
            Editor::GetState().SelectedEntity = entityId == -1.f ? Heart::Entity() : Heart::Entity(&Editor::GetActiveScene(), static_cast<u32>(entityId));
        }

        return true;
    }

    bool EditorLayer::MouseButtonReleasedEvent(Heart::MouseButtonReleasedEvent& event)
    {
        return false;
    }
}
