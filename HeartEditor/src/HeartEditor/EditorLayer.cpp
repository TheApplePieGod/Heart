#include "htpch.h"
#include "EditorLayer.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Input/Input.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "imgui/imgui_internal.h"
#include "glm/vec4.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.hpp"

namespace HeartEditor
{
    EditorLayer::EditorLayer()
    {
        // Register required editor resources
        // Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Texture, "editor/pan.png", true, true);
        // Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Texture, "editor/rotate.png", true, true);
        // Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Texture, "editor/scale.png", true, true);
        // Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Texture, "editor/object.png", true, true);
        // Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Texture, "editor/world.png", true, true);
        // Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Texture, "editor/folder.png", true, true);
        // Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Texture, "editor/file.png", true, true);

        // Register testing assets
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Material, "assets/materials/TestMaterial.hemat");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Texture, "assets/textures/fish.png");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Texture, "assets/textures/test.png");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/cube.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/Sponza/glTF/Sponza.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/Buggy/glTF/Buggy.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/Duck/glTF/Duck.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/Lantern/glTF/Lantern.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/BoomBox/glTF/BoomBox.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/GlamVelvetSofa/glTF/GlamVelvetSofa.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/Avocado/glTF/Avocado.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/DragonAttenuation/glTF/DragonAttenuation.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/Suzanne/glTF/Suzanne.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/WaterBottle/glTF/WaterBottle.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/StainedGlassLamp/glTF/StainedGlassLamp.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/SciFiHelmet/glTF/SciFiHelmet.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/RecursiveSkeletons/glTF/RecursiveSkeletons.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/MosquitoInAmber/glTF/MosquitoInAmber.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/SheenChair/glTF/SheenChair.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/DamagedHelmet/glTF/DamagedHelmet.gltf");
        Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, "assets/meshes/IridescentDishWithOlives/glTF/IridescentDishWithOlives.gltf");

        m_EditorCamera = Heart::CreateScope<EditorCamera>(70.f, 0.1f, 500.f, 1.f);
        m_ActiveScene = Heart::CreateRef<Heart::Scene>();

        auto entity = m_ActiveScene->CreateEntity("Test Entity");
        entity.AddComponent<Heart::MeshComponent>();
        entity.GetComponent<Heart::MeshComponent>().Mesh = Heart::AssetManager::GetAssetUUID("assets/meshes/Sponza/glTF/Sponza.gltf");

        //entity = m_ActiveScene->CreateEntity("Cube Entity");
        //entity.AddComponent<Heart::MeshComponent>();
        //entity.GetComponent<Heart::MeshComponent>().Mesh = Heart::AssetManager::GetAssetUUID("assets/meshes/DamagedHelmet/glTF/DamagedHelmet.gltf");
        //entity.GetComponent<Heart::MeshComponent>().Mesh = Heart::AssetManager::GetAssetUUID("assets/meshes/Buggy/glTF/Buggy.gltf");

        m_EnvironmentMaps.emplace_back(Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Texture, "assets/envmaps/GrandCanyon.hdr"));
        //m_EnvironmentMaps.emplace_back(Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Texture, "assets/envmaps/IceLake.hdr"));
        //m_EnvironmentMaps.emplace_back(Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Texture, "assets/envmaps/PopcornLobby.hdr"));
        //m_EnvironmentMaps.emplace_back(Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Texture, "assets/envmaps/Factory.hdr"));
        //m_EnvironmentMaps.emplace_back(Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Texture, "assets/envmaps/Bridge.hdr"));
    }

    EditorLayer::~EditorLayer()
    {
        
    }

    void EditorLayer::OnAttach()
    {
        HE_PROFILE_FUNCTION();

        SubscribeToEmitter(&EditorApp::Get().GetWindow());

        m_SceneRenderer = Heart::CreateScope<Heart::SceneRenderer>();

        auto loadTimer = Heart::Timer("Environment map generation");
        for (auto& map : m_EnvironmentMaps)
        {
            map.Initialize();
            map.Recalculate();
        }

        m_Widgets.MaterialEditor.Initialize();

        HE_CLIENT_LOG_INFO("Editor attached");
    }

    void EditorLayer::OnUpdate(Heart::Timestep ts)
    {
        HE_PROFILE_FUNCTION();
        auto timer = Heart::AggregateTimer("EditorLayer::OnUpdate");

        m_EditorCamera->OnUpdate(ts, m_ViewportInput, m_ViewportHover);

        m_SceneRenderer->RenderScene(
            EditorApp::Get().GetWindow().GetContext(),
            m_ActiveScene.get(),
            *m_EditorCamera,
            m_EditorCamera->GetPosition(),
            &m_EnvironmentMaps[0]
        );
    }

    void EditorLayer::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();
        auto timer = Heart::AggregateTimer("EditorLayer::OnImGuiRender");
        ImGuizmo::BeginFrame();

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::Begin("Main Window", nullptr, windowFlags);
        
        m_Widgets.MainMenuBar.OnImGuiRender(m_ActiveScene.get(), m_SelectedEntity);

        ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.f, 0.f), 0);

        if (m_Widgets.MainMenuBar.GetWindowStatus("Viewport"))
        {
            ImGui::Begin("Viewport", m_Widgets.MainMenuBar.GetWindowStatusRef("Viewport"));

            RenderViewport();

            ImGui::End();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("Content Browser"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
            ImGui::Begin("Content Browser", m_Widgets.MainMenuBar.GetWindowStatusRef("Content Browser"));

            m_Widgets.ContentBrowser.OnImGuiRender();

            ImGui::End();
            ImGui::PopStyleVar();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("Scene Hierarchy"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
            ImGui::Begin("Scene Hierarchy", m_Widgets.MainMenuBar.GetWindowStatusRef("Scene Hierarchy"));

            m_Widgets.SceneHierarchyPanel.OnImGuiRender(m_ActiveScene.get(), m_SelectedEntity);

            ImGui::End();
            ImGui::PopStyleVar();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("Properties Panel"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
            ImGui::Begin("Properties", m_Widgets.MainMenuBar.GetWindowStatusRef("Properties Panel"));

            m_Widgets.PropertiesPanel.OnImGuiRender(m_SelectedEntity);

            ImGui::End();
            ImGui::PopStyleVar();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("Settings"))
        {
            ImGui::Begin("Settings", m_Widgets.MainMenuBar.GetWindowStatusRef("Settings"));
            ImGui::End();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("Debug Info"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
            ImGui::Begin("Debug Info", m_Widgets.MainMenuBar.GetWindowStatusRef("Debug Info"));

            RenderDebugInfo();

            ImGui::End();
            ImGui::PopStyleVar();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("Material Editor"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
            ImGui::Begin("Material Editor", m_Widgets.MainMenuBar.GetWindowStatusRef("Material Editor"));

            m_Widgets.MaterialEditor.OnImGuiRender(&m_EnvironmentMaps[0]);

            ImGui::End();
            ImGui::PopStyleVar();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("ImGui Demo"))
            ImGui::ShowDemoWindow(m_Widgets.MainMenuBar.GetWindowStatusRef("ImGui Demo"));

        ImGui::End();

        ImGui::PopStyleVar(2);
    }

    void EditorLayer::RenderViewport()
    {
        HE_PROFILE_FUNCTION();
        
        // calculate viewport bounds & aspect ratio
        ImVec2 viewportMin = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMax = ImGui::GetWindowContentRegionMax();
        ImVec2 viewportPos = ImGui::GetWindowPos();
        m_ViewportSize = { viewportMax.x - viewportMin.x, viewportMax.y - viewportMin.y };
        glm::vec2 viewportStart = { viewportMin.x + viewportPos.x, viewportMin.y + viewportPos.y };
        glm::vec2 viewportEnd = viewportStart + m_ViewportSize;
        m_EditorCamera->UpdateAspectRatio(m_ViewportSize.x / m_ViewportSize.y);
        m_ViewportMousePos = glm::vec2(std::clamp(ImGui::GetMousePos().x - viewportStart.x, 0.f, m_ViewportSize.x), std::clamp(ImGui::GetMousePos().y - viewportStart.y, 0.f, m_ViewportSize.y));

        // draw the viewport background
        ImGui::GetWindowDrawList()->AddRectFilled({ viewportStart.x, viewportStart.y }, { viewportEnd.x, viewportEnd.y }, IM_COL32( 0, 0, 0, 255 )); // viewport background

        // initialize imguizmo & draw the grid
        glm::mat4 view = m_EditorCamera->GetViewMatrixInvertedY();
        glm::mat4 proj = m_EditorCamera->GetProjectionMatrix();
        glm::mat4 identity(1.f);
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(viewportStart.x, viewportStart.y, m_ViewportSize.x, m_ViewportSize.y);
        //ImGuizmo::DrawGrid(glm::value_ptr(view), glm::value_ptr(proj), glm::value_ptr(identity), 100.f);

        // draw the rendered texture
        ImGui::Image(
            m_SceneRenderer->GetFinalFramebuffer().GetColorAttachmentImGuiHandle(0),
            { m_ViewportSize.x, m_ViewportSize.y }
        );

        // enable input if the viewport is being right clicked
        m_ViewportHover = ImGui::IsItemHovered();
        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(1))
        {
            // disable imgui input & cursor
            m_ViewportInput = true;
            EditorApp::Get().GetWindow().DisableCursor();
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
            ImGui::SetWindowFocus();
        }

        // on-screen buttons
        ImGui::SetCursorPos(viewportMin);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.f, 0.f));
        if (ImGui::BeginTable("GizmoOperations", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextRow();

            //ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.2f, 0.2f, 0.2f, 0.5f)));

            ImGui::TableSetColumnIndex(0);
            if (ImGui::ImageButton(
                Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>("editor/pan.png", true)->GetTexture()->GetImGuiHandle(),
                { 25, 25 }
            ))
            { m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE; }
            RenderTooltip("Change gizmo to translate mode");

            ImGui::TableSetColumnIndex(1);
            if (ImGui::ImageButton(
                Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>("editor/rotate.png", true)->GetTexture()->GetImGuiHandle(),
                { 25, 25 }
            ))
            { m_GizmoOperation = ImGuizmo::OPERATION::ROTATE; }
            RenderTooltip("Change gizmo to rotate mode");

            ImGui::TableSetColumnIndex(2);
            if (ImGui::ImageButton(
                Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>("editor/scale.png", true)->GetTexture()->GetImGuiHandle(),
                { 25, 25 }
            ))
            { m_GizmoOperation = ImGuizmo::OPERATION::SCALE; }
            RenderTooltip("Change gizmo to scale mode");

            ImGui::TableSetColumnIndex(3);
            ImGui::Dummy({ 15.f, 0.f });

            ImGui::TableSetColumnIndex(4);
            if (ImGui::ImageButton(
                Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(m_GizmoMode ? "editor/world.png" : "editor/object.png", true)->GetTexture()->GetImGuiHandle(),
                { 25, 25 }
            ))
            { m_GizmoMode = (ImGuizmo::MODE)(!m_GizmoMode); }
            RenderTooltip(m_GizmoMode ? "Change gizmo to operate in local space" : "Change gizmo to operate in world space");

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        // hover is false if we are hovering over the buttons
        m_ViewportHover = m_ViewportHover && !ImGui::IsItemHovered();

        // draw the imguizmo if an entity is selected
        if (m_SelectedEntity.IsValid() && m_SelectedEntity.HasComponent<Heart::TransformComponent>())
        {
            auto& transformComponent = m_SelectedEntity.GetComponent<Heart::TransformComponent>();
            glm::mat4 parentTransform;
            glm::mat4 transform = m_ActiveScene->CalculateEntityTransform(m_SelectedEntity, &parentTransform);

            ImGuizmo::Manipulate(
                glm::value_ptr(view),
                glm::value_ptr(proj),
                m_GizmoOperation,
                m_GizmoMode,
                glm::value_ptr(transform),
                nullptr,
                nullptr,
                nullptr,
                nullptr
            );

            if (ImGuizmo::IsUsing())
            {
                glm::vec3 invskew;
                glm::vec4 invperspective;
                glm::quat invrotation;
            
                // convert the word space transform to local space by multiplying it by the inverse of the parent transform
                glm::decompose(glm::inverse(parentTransform) * transform, transformComponent.Scale, invrotation, transformComponent.Translation, invskew, invperspective);
                transformComponent.Rotation = glm::degrees(glm::eulerAngles(invrotation));

                m_ActiveScene->CacheEntityTransform(m_SelectedEntity);
            }
        }
    }

    void EditorLayer::RenderDebugInfo()
    {
        double stepMs = EditorApp::Get().GetLastTimestep().StepMilliseconds();
        ImGui::Text("Render Api: %s", HE_ENUM_TO_STRING(Heart::RenderApi, Heart::Renderer::GetApiType()));
        ImGui::Text("Frametime: %.1fms", stepMs);
        ImGui::Text("Framerate: %d FPS", static_cast<u32>(1000.0 / stepMs));

        ImGui::Separator();

        glm::vec3 cameraPos = m_EditorCamera->GetPosition();
        glm::vec3 cameraFor = m_EditorCamera->GetForwardVector();
        ImGui::Text("Camera Pos: (%.2f, %.2f, %.2f)", cameraPos.x, cameraPos.y, cameraPos.z);
        ImGui::Text("Camera Dir: (%.2f, %.2f, %.2f)", cameraFor.x, cameraFor.y, cameraFor.z);
        ImGui::Text("Camera Rot: (%.2f, %.2f)", m_EditorCamera->GetXRotation(), m_EditorCamera->GetYRotation());
        ImGui::Text("Mouse Pos: (%.1f, %.1f)", Heart::Input::GetScreenMousePos().x, Heart::Input::GetScreenMousePos().y);
        ImGui::Text("VP Mouse: (%.1f, %.1f)", m_ViewportMousePos.x, m_ViewportMousePos.y);
        ImGui::Text("VP Hover: %s", m_ViewportHover ? "true" : "false");

        for (auto& pair : Heart::AggregateTimer::GetTimeMap())
            ImGui::Text("%s: %.1fms", pair.first.c_str(), pair.second);
    }

    void EditorLayer::RenderTooltip(const std::string& text)
    {
        if (ImGui::IsItemHovered())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5.f, 5.f} );
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(text.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
            ImGui::PopStyleVar();
        }
    }

    void EditorLayer::OnDetach()
    {
        UnsubscribeFromEmitter(&EditorApp::Get().GetWindow());
        
        m_SceneRenderer.reset();

        for (auto& map : m_EnvironmentMaps)
            map.Shutdown();

        m_Widgets.MaterialEditor.Shutdown();

        HE_CLIENT_LOG_INFO("Editor detached");
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
        {
            if (!m_ViewportInput)
                EditorApp::Get().Close();
        }
        else if (event.GetKeyCode() == Heart::KeyCode::F11)
            EditorApp::Get().GetWindow().ToggleFullscreen();
        
        return true;
    }

    bool EditorLayer::MouseButtonPressedEvent(Heart::MouseButtonPressedEvent& event)
    {
        // screen picking
        if (event.GetMouseCode() == Heart::MouseCode::LeftButton && (!ImGuizmo::IsOver() || !m_SelectedEntity.IsValid()) && !m_ViewportInput && m_ViewportHover)
        {
            // the image is scaled down in the viewport, so we need to adjust what pixel we are sampling from
            u32 sampleX = static_cast<u32>(m_ViewportMousePos.x / m_ViewportSize.x * m_SceneRenderer->GetFinalFramebuffer().GetWidth());
            u32 sampleY = static_cast<u32>(m_ViewportMousePos.y / m_ViewportSize.y * m_SceneRenderer->GetFinalFramebuffer().GetHeight());

            f32 entityId = m_SceneRenderer->GetFinalFramebuffer().ReadColorAttachmentPixel<f32>(1, sampleX, sampleY, 0);
            m_SelectedEntity = entityId == -1.f ? Heart::Entity() : Heart::Entity(m_ActiveScene.get(), static_cast<u32>(entityId));
        }

        return true;
    }

    bool EditorLayer::MouseButtonReleasedEvent(Heart::MouseButtonReleasedEvent& event)
    {
        if (event.GetMouseCode() == Heart::MouseCode::RightButton)
        {
            m_ViewportInput = false;
            EditorApp::Get().GetWindow().EnableCursor();
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        }

        return true;
    }
}