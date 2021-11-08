#include "htpch.h"
#include "EditorLayer.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Input/Input.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "glm/vec4.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "imguizmo/ImGuizmo.h"

namespace HeartEditor
{
    EditorLayer::EditorLayer()
    {
        m_EditorCamera = Heart::CreateScope<EditorCamera>(70.f, 0.1f, 1000.f, 1.f);
        m_ActiveScene = Heart::CreateRef<Heart::Scene>();

        auto entity = m_ActiveScene->CreateEntity("Test Entity");
        entity.AddComponent<Heart::MeshComponent>();
    }

    EditorLayer::~EditorLayer()
    {
        
    }

    void EditorLayer::OnAttach()
    {
        HE_PROFILE_FUNCTION();

        SubscribeToEmitter(&EditorApp::Get().GetWindow());

        m_SceneRenderer = Heart::CreateScope<Heart::SceneRenderer>();

        HE_CLIENT_LOG_INFO("Editor attached");
    }

    void EditorLayer::OnUpdate(Heart::Timestep ts)
    {
        HE_PROFILE_FUNCTION();

        auto timer = Heart::AggregateTimer("EditorLayer::OnUpdate");

        if (m_ViewportInput)
            m_EditorCamera->OnUpdate(ts);

        m_SceneRenderer->RenderScene(EditorApp::Get().GetWindow().GetContext(), m_ActiveScene.get(), m_EditorCamera->GetViewProjectionMatrix());
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
        
        m_Widgets.MainMenuBar.OnImGuiRender();

        ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.f, 0.f), 0);

        if (m_Widgets.MainMenuBar.GetWindowStatus("Viewport"))
        {
            ImGui::Begin("Viewport", m_Widgets.MainMenuBar.GetWindowStatusRef("Viewport"));

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
            ImGuizmo::DrawGrid(glm::value_ptr(view), glm::value_ptr(proj), glm::value_ptr(identity), 100.f);

            // draw the rendered texture
            ImGui::Image(
                m_SceneRenderer->GetFinalFramebuffer().GetColorAttachmentImGuiHandle(0),
                { m_ViewportSize.x, m_ViewportSize.y },
                { 0.f, 0.f }, { 1.f, 1.f }
            );

            // enable input if the viewport is being right clicked
            m_ViewportHover = ImGui::IsItemHovered();
            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(1))
            {
                // disable imgui input & cursor
                m_ViewportInput = true;
                EditorApp::Get().GetWindow().DisableCursor();
                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
            }
            ImGui::SetItemAllowOverlap();

            // draw the imguizmo if an entity is selected
            if (m_Widgets.SceneHierarchyPanel.GetSelectedEntity().IsValid() && m_Widgets.SceneHierarchyPanel.GetSelectedEntity().HasComponent<Heart::TransformComponent>())
            {
                auto& transformComponent = m_Widgets.SceneHierarchyPanel.GetSelectedEntity().GetComponent<Heart::TransformComponent>();
                glm::mat4 transform = transformComponent.GetTransformMatrix();
                ImGuizmo::Manipulate(
                    glm::value_ptr(view),
                    glm::value_ptr(proj),
                    ImGuizmo::OPERATION::TRANSLATE,
                    ImGuizmo::MODE::LOCAL,
                    glm::value_ptr(transform),
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr
                );

                if (ImGuizmo::IsUsing())
                {
                    glm::vec3 skew;
                    glm::vec4 perspective;
                    glm::quat rotation;
                    glm::decompose(transform, transformComponent.Scale, rotation, transformComponent.Translation, skew, perspective);
                    transformComponent.Rotation = glm::degrees(glm::eulerAngles(rotation));
                }
            }

            ImGui::End();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("Content Browser"))
        {
            ImGui::Begin("Content Browser", m_Widgets.MainMenuBar.GetWindowStatusRef("Content Browser"));

            ImGui::End();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("Scene Hierarchy"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
            ImGui::Begin("Scene Hierarchy", m_Widgets.MainMenuBar.GetWindowStatusRef("Scene Hierarchy"));

            m_Widgets.SceneHierarchyPanel.OnImGuiRender(m_ActiveScene.get());

            ImGui::End();
            ImGui::PopStyleVar();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("Properties Panel"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
            ImGui::Begin("Properties", m_Widgets.MainMenuBar.GetWindowStatusRef("Properties Panel"));

            m_Widgets.PropertiesPanel.OnImGuiRender(m_Widgets.SceneHierarchyPanel.GetSelectedEntity());

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

            ImGui::Text("Render Api: %s", HE_ENUM_TO_STRING(Heart::RenderApi, Heart::Renderer::GetApiType()));

            glm::vec3 cameraPos = m_EditorCamera->GetPosition();
            glm::vec3 cameraFor = m_EditorCamera->GetForwardVector();
            ImGui::Text("Camera Pos: (%.2f, %.2f, %.2f)", cameraPos.x, cameraPos.y, cameraPos.z);
            ImGui::Text("Camera Dir: (%.2f, %.2f, %.2f)", cameraFor.x, cameraFor.y, cameraFor.z);
            ImGui::Text("Camera Rot: (%.2f, %.2f)", m_EditorCamera->GetXRotation(), m_EditorCamera->GetYRotation());
            ImGui::Text("Mouse Pos: (%.1f, %.1f)", Heart::Input::GetScreenMousePos().x, Heart::Input::GetScreenMousePos().y);
            ImGui::Text("VP Mouse: (%.1f, %.1f)", m_ViewportMousePos.x, m_ViewportMousePos.y);

            for (auto& pair : Heart::AggregateTimer::GetTimeMap())
                ImGui::Text("%s: %dms", pair.first.c_str(), pair.second);

            ImGui::End();
            ImGui::PopStyleVar();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("ImGui Demo"))
        {
            ImGui::ShowDemoWindow(m_Widgets.MainMenuBar.GetWindowStatusRef("ImGui Demo"));
        }

        ImGui::End();

        ImGui::PopStyleVar(2);
    }

    void EditorLayer::OnDetach()
    {
        UnsubscribeFromEmitter(&EditorApp::Get().GetWindow());
        
        m_SceneRenderer.reset();

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
        if (event.GetMouseCode() == Heart::MouseCode::LeftButton && !ImGuizmo::IsOver() && !m_ViewportInput && m_ViewportHover)
        {
            // the image is scaled down in the viewport, so we need to adjust what pixel we are sampling from
            u32 sampleX = static_cast<u32>(m_ViewportMousePos.x / m_ViewportSize.x * m_SceneRenderer->GetFinalFramebuffer().GetWidth());
            u32 sampleY = static_cast<u32>(m_ViewportMousePos.y / m_ViewportSize.y * m_SceneRenderer->GetFinalFramebuffer().GetHeight());

            f32 entityId = m_SceneRenderer->GetFinalFramebuffer().ReadAttachmentPixel<f32>(1, sampleX, sampleY, 0);
            Heart::Entity entity = entityId == -1.f ? Heart::Entity() : Heart::Entity(m_ActiveScene.get(), static_cast<u32>(entityId));
            m_Widgets.SceneHierarchyPanel.SetSelectedEntity(entity);
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