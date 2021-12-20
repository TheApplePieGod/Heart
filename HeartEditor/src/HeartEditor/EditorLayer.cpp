#include "hepch.h"
#include "EditorLayer.h"

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

#include "HeartEditor/Widgets/SceneHierarchyPanel.h"
#include "HeartEditor/Widgets/PropertiesPanel.h"
#include "HeartEditor/Widgets/ContentBrowser.h"
#include "HeartEditor/Widgets/MaterialEditor.h"
#include "HeartEditor/Widgets/Viewport.h"
#include "HeartEditor/Widgets/DebugInfo.h"
#include "HeartEditor/Widgets/Settings.h"
#include "HeartEditor/Widgets/SceneSettings.h"

namespace HeartEditor
{
    EditorLayer::EditorLayer()
    {
        Editor::Initialize();

        // Register widgets
        Editor::PushWindow(
            "Viewport",
            Heart::CreateRef<Widgets::Viewport>("Viewport", true)
        );
        Editor::PushWindow(
            "Content Browser",
            Heart::CreateRef<Widgets::ContentBrowser>("Content Browser", true)
        );
        Editor::PushWindow(
            "Scene Hierarchy",
            Heart::CreateRef<Widgets::SceneHierarchyPanel>("Scene Hierarchy", true)
        );
        Editor::PushWindow(
            "Properties Panel",
            Heart::CreateRef<Widgets::PropertiesPanel>("Properties Panel", true)
        );
        Editor::PushWindow(
            "Material Editor",
            Heart::CreateRef<Widgets::MaterialEditor>("Material Editor", false)
        );
        Editor::PushWindow(
            "Debug Info",
            Heart::CreateRef<Widgets::DebugInfo>("Debug Info", true)
        );
        Editor::PushWindow(
            "Settings",
            Heart::CreateRef<Widgets::Settings>("Settings", true)
        );
        Editor::PushWindow(
            "Scene Settings",
            Heart::CreateRef<Widgets::SceneSettings>("Scene Settings", true)
        );

        // auto entity = Editor::GetActiveScene().CreateEntity("Sponza");
        // entity.AddComponent<Heart::MeshComponent>();
        // entity.GetComponent<Heart::MeshComponent>().Mesh = Heart::AssetManager::GetAssetUUID("assets/meshes/Sponza/glTF/Sponza.gltf");

        // auto entity = Editor::GetActiveScene().CreateEntity("Cube Entity");
        // entity.AddComponent<Heart::MeshComponent>();
        // entity.GetComponent<Heart::MeshComponent>().Mesh = Heart::AssetManager::GetAssetUUID("DefaultCube.gltf", true);

        // auto& lightComp = entity.AddComponent<Heart::LightComponent>();
        // lightComp.Color.a = 100.f;
        // lightComp.LightType = Heart::LightComponent::Type::Directional;

        // int max = 10;
        // for (int i = 0; i < 5; i++)
        // {
        //     Heart::Entity entity = Editor::GetActiveScene().CreateEntity("Light " + std::to_string(i));
        //     auto& comp = entity.AddComponent<Heart::PointLightComponent>();
        //     //comp.LinearAttenuation = 1.f;
        //     comp.Color = { (rand() % 255) / 255.f, (rand() % 255) / 255.f, (rand() % 255) / 255.f, rand() % 50 };
        //     entity.SetPosition({ rand() % (max * 2) - max, rand() % (max * 2) - max, rand() % (max * 2) - max });
        // }

        int max = 100;
        for (int i = 0; i < 200; i++)
        {
            Heart::Entity entity = Editor::GetActiveScene().CreateEntity("Mesh " + std::to_string(i));
            auto& comp = entity.AddComponent<Heart::MeshComponent>().Mesh = Heart::AssetManager::GetAssetUUID("assets/meshes/Sponza/glTF/Sponza.gltf");
            entity.SetPosition({ rand() % (max * 2) - max, rand() % (max * 2) - max, rand() % (max * 2) - max });
        }
    }

    EditorLayer::~EditorLayer()
    {
        Editor::Shutdown();
    }

    void EditorLayer::OnAttach()
    {
        HE_PROFILE_FUNCTION();

        SubscribeToEmitter(&EditorApp::Get().GetWindow());

        HE_CLIENT_LOG_INFO("Editor attached");
    }

    void EditorLayer::OnDetach()
    {
        UnsubscribeFromEmitter(&EditorApp::Get().GetWindow());

        ((Widgets::MaterialEditor&)Editor::GetWindow("Material Editor")).Reset();

        HE_CLIENT_LOG_INFO("Editor detached");
    }

    void EditorLayer::OnUpdate(Heart::Timestep ts)
    {

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
        if (event.GetKeyCode() == Heart::KeyCode::Escape)
            EditorApp::Get().Close();
        else if (event.GetKeyCode() == Heart::KeyCode::F11)
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