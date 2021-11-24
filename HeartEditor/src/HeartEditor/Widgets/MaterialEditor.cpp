#include "htpch.h"
#include "MaterialEditor.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Scene/Components.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Input/Input.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "Heart/Util/ImGuiUtil.h"

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
        
        Heart::ImGuiUtil::ResizableWindowSplitter(
            m_WindowSizes,
            { 100.f, 100.f },
            true,
            6.f,
            10.f,
            [&]() { RenderSidebar(); },
            [&, envMap]() { RenderViewport(envMap); }
        );
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