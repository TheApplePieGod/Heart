#include "htpch.h"
#include "MaterialEditor.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Scene/Components.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Input/Input.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace HeartEditor
{
namespace Widgets
{
    MaterialEditor::MaterialEditor()
    {
        m_Scene = Heart::CreateRef<Heart::Scene>();

        m_DemoEntity = m_Scene->CreateEntity("Demo Entity");
        m_DemoEntity.AddComponent<Heart::MeshComponent>();
        m_DemoEntity.GetComponent<Heart::MeshComponent>().Mesh = Heart::AssetManager::GetAssetUUID("DefaultCube.gltf", true);

        m_SceneCamera.UpdateViewMatrix({ 0.f, 0.f, 0.f }, m_Radius, 0, 0);
        m_SceneCameraPosition = -m_SceneCamera.GetForwardVector() * m_Radius;
    }

    void MaterialEditor::Initialize()
    {
        m_SceneRenderer = Heart::CreateScope<Heart::SceneRenderer>();
    }

    void MaterialEditor::Shutdown()
    {
        m_SceneRenderer.reset();
    }

    void MaterialEditor::OnImGuiRender(Heart::EnvironmentMap* envMap)
    {
        HE_PROFILE_FUNCTION();
        
        f32 windowWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
        f32 windowHeight = ImGui::GetContentRegionAvail().y;

        // update the 'max size'
        m_WindowSizes.y = windowWidth - m_WindowSizes.x - 16.f;

        // draw the splitter that allows for child window resizing
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;
        bb.Min = ImVec2(window->DC.CursorPos.x + m_WindowSizes.x, window->DC.CursorPos.y);
        bb.Max = ImGui::CalcItemSize(ImVec2(6.f, windowHeight), 0.0f, 0.0f);
        bb.Max = { bb.Max.x + bb.Min.x, bb.Max.y + bb.Min.y };
        ImGui::SplitterBehavior(bb, id, ImGuiAxis_X, &m_WindowSizes.x, &m_WindowSizes.y, 100.f, 100.f, 0.0f);

        // tree
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::BeginChild("cbtree", ImVec2(m_WindowSizes.x, windowHeight), false);
        
        RenderSidebar();

        ImGui::EndChild();
        
        ImGui::SameLine(0.f, 10.f);

        ImGui::BeginChild("cbfiles", ImVec2(m_WindowSizes.y, windowHeight), false);
        
        RenderViewport(envMap);

        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    void MaterialEditor::RenderSidebar()
    {
        ImGui::Text("test");
    }

    void MaterialEditor::RenderViewport(Heart::EnvironmentMap* envMap)
    {
        // calculate viewport bounds & aspect ratio
        ImVec2 viewportMin = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMax = ImGui::GetWindowContentRegionMax();
        ImVec2 viewportPos = ImGui::GetWindowPos();
        glm::vec2 viewportSize = { viewportMax.x - viewportMin.x, viewportMax.y - viewportMin.y };
        glm::vec2 viewportStart = { viewportMin.x + viewportPos.x, viewportMin.y + viewportPos.y };
        glm::vec2 viewportEnd = viewportStart + viewportSize;
        m_SceneCamera.UpdateAspectRatio(viewportSize.x / viewportSize.y);

        m_SceneRenderer->RenderScene(
            EditorApp::Get().GetWindow().GetContext(),
            m_Scene.get(),
            m_SceneCamera,
            m_SceneCameraPosition,
            envMap
        );

        // draw the viewport background
        ImGui::GetWindowDrawList()->AddRectFilled({ viewportStart.x, viewportStart.y }, { viewportEnd.x, viewportEnd.y }, IM_COL32( 0, 0, 0, 255 )); // viewport background

        // draw the rendered texture
        ImGui::Image(
            m_SceneRenderer->GetFinalFramebuffer().GetColorAttachmentImGuiHandle(0),
            { viewportSize.x, viewportSize.y }
        );
        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseDragging(0))
            {
                m_SwivelRotation.x += static_cast<float>(Heart::Input::GetMouseDeltaX());
                m_SwivelRotation.y += static_cast<float>(Heart::Input::GetMouseDeltaY());
            }
            m_Radius = std::clamp(m_Radius + static_cast<float>(-Heart::Input::GetScrollOffsetY()), 0.f, 100.f);
            m_SceneCamera.UpdateViewMatrix({ 0.f, 0.f, 0.f }, m_Radius, m_SwivelRotation.x, m_SwivelRotation.y);
            m_SceneCameraPosition = -m_SceneCamera.GetForwardVector() * m_Radius;
        }
    }
}
}