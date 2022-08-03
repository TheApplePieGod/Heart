#include "hepch.h"
#include "EditorLayer.h"

#include "HeartEditor/Widgets/Viewport.h"
#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "Heart/Core/Window.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Input/Input.h"
#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Util/ImGuiUtils.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Asset/SceneAsset.h"
#include "Heart/Events/KeyboardEvents.h"
#include "Heart/Events/MouseEvents.h"
#include "imgui/imgui_internal.h"


namespace HeartEditor
{
    EditorLayer::EditorLayer()
    {
        Editor::Initialize();
    }

    EditorLayer::~EditorLayer()
    {
        Editor::Shutdown();
    }

    void EditorLayer::OnAttach()
    {
        HE_PROFILE_FUNCTION();

        SubscribeToEmitter(&EditorApp::Get().GetWindow());

        Editor::CreateWindows();

        HE_LOG_INFO("Editor attached");
    }

    void EditorLayer::OnDetach()
    {
        UnsubscribeFromEmitter(&EditorApp::Get().GetWindow());

        Editor::DestroyWindows();

        HE_LOG_INFO("Editor detached");
    }

    void EditorLayer::OnUpdate(Heart::Timestep ts)
    {
        if (Editor::GetSceneState() == SceneState::Playing)
            Editor::GetActiveScene().OnUpdateRuntime(ts);
    }

    void EditorLayer::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();
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

        ImGui::End();

        ImGui::PopStyleVar(2);
    }

    void EditorLayer::OnEvent(Heart::Event& event)
    {
        event.Map<Heart::KeyPressedEvent>(HE_BIND_EVENT_FN(EditorLayer::KeyPressedEvent));
        event.Map<Heart::MouseButtonPressedEvent>(HE_BIND_EVENT_FN(EditorLayer::MouseButtonPressedEvent));
        event.Map<Heart::MouseButtonReleasedEvent>(HE_BIND_EVENT_FN(EditorLayer::MouseButtonReleasedEvent));
    }

    bool EditorLayer::KeyPressedEvent(Heart::KeyPressedEvent& event)
    {
        // if (event.GetKeyCode() == Heart::KeyCode::Escape)
        //     EditorApp::Get().Close();
        if (event.GetKeyCode() == Heart::KeyCode::F11)
            EditorApp::Get().GetWindow().ToggleFullscreen();
        
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

            // the image is scaled down in the viewport, so we need to adjust what pixel we are sampling from
            u32 sampleX = static_cast<u32>(mousePos.x / size.x * viewport.GetSceneRenderer().GetEntityIdsTexture().GetWidth());
            u32 sampleY = static_cast<u32>(mousePos.y / size.y * viewport.GetSceneRenderer().GetEntityIdsTexture().GetHeight());

            f32 entityId = viewport.GetSceneRenderer().GetEntityIdsTexture().ReadPixel<f32>(sampleX, sampleY, 0);
            Editor::GetState().SelectedEntity = entityId == -1.f ? Heart::Entity() : Heart::Entity(&Editor::GetActiveScene(), static_cast<u32>(entityId));
        }

        return true;
    }

    bool EditorLayer::MouseButtonReleasedEvent(Heart::MouseButtonReleasedEvent& event)
    {
        auto& viewport = (Widgets::Viewport&)Editor::GetWindow("Viewport");
        if (event.GetMouseCode() == Heart::MouseCode::RightButton)
        {
            viewport.SetFocused(false);
            EditorApp::Get().GetWindow().EnableCursor();
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        }

        return true;
    }
}