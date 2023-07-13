#include "hepch.h"
#include "Viewport.h"

#include "HeartRuntime/RuntimeApp.h"
#include "Heart/Core/Camera.h"
#include "Heart/Core/Window.h"
#include "Heart/Input/Input.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Entity.h"
#include "Flourish/Api/Context.h"
#include "Flourish/Api/Texture.h"

namespace HeartRuntime
{
    Viewport::Viewport()
    {
        m_SceneRenderer = Heart::CreateScope<Heart::SceneRenderer>();
    }

    void Viewport::Shutdown()
    {
        m_SceneRenderer.reset();
    }

    void Viewport::OnImGuiRender(
            Heart::RenderScene* renderScene,
            Heart::Scene* sceneContext,
            const Heart::SceneRenderSettings& settings)
    {
        HE_PROFILE_FUNCTION();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 viewportPos = viewport->WorkPos;
        ImVec2 viewportSize = viewport->WorkSize;
        ImGui::SetNextWindowPos(viewportPos);
        ImGui::SetNextWindowSize(viewportSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
        ImGui::Begin("##viewport", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

        glm::vec3 cameraPosition;
        float aspectRatio = viewportSize.x / viewportSize.y;
        auto primaryCamEntity = sceneContext->GetPrimaryCameraEntity();
        if (primaryCamEntity.IsValid())
        {
            auto& camComp = primaryCamEntity.GetComponent<Heart::CameraComponent>();
            m_Camera = Heart::Camera(
                camComp.FOV,
                camComp.NearClipPlane,
                camComp.FarClipPlane,
                aspectRatio
            );
            cameraPosition = primaryCamEntity.GetWorldPosition();
            m_Camera.UpdateViewMatrix(cameraPosition, primaryCamEntity.GetWorldRotation());
        }
        else
        {
            f32 moveSpeed = 2.f; // m/s
            f32 mouseScale = 0.1f;
            auto ts = RuntimeApp::Get().GetLastTimestep();
            if (Heart::Input::IsKeyPressed(Heart::KeyCode::A))
                m_DebugCameraPos -= (m_Camera.GetRightVector() * moveSpeed * static_cast<f32>(ts.StepSeconds()));
            if (Heart::Input::IsKeyPressed(Heart::KeyCode::D))
                m_DebugCameraPos += (m_Camera.GetRightVector() * moveSpeed * static_cast<f32>(ts.StepSeconds()));
            if (Heart::Input::IsKeyPressed(Heart::KeyCode::W))
                m_DebugCameraPos += (m_Camera.GetForwardVector() * moveSpeed * static_cast<f32>(ts.StepSeconds()));
            if (Heart::Input::IsKeyPressed(Heart::KeyCode::S))
                m_DebugCameraPos -= (m_Camera.GetForwardVector() * moveSpeed * static_cast<f32>(ts.StepSeconds()));

            m_DebugCameraRot.y += mouseScale * static_cast<f32>(Heart::Input::GetMouseDeltaX());
            m_DebugCameraRot.x += mouseScale * static_cast<f32>(Heart::Input::GetMouseDeltaY());

            m_Camera = Heart::Camera(
                70.f,
                0.1f,
                500.f,
                aspectRatio
            );
            m_Camera.UpdateViewMatrix(m_DebugCameraPos, m_DebugCameraRot);
            cameraPosition = m_DebugCameraPos;
        }

        auto renderGroup = m_SceneRenderer->Render({
            renderScene,
            sceneContext->GetEnvironmentMap(),
            &m_Camera,
            cameraPosition,
            settings
        });

        // TODO: don't need to wait right away
        renderGroup.Wait();

        Flourish::Context::PushFrameRenderGraph(m_SceneRenderer->GetRenderGraph());

        // draw the viewport background
        ImGui::GetWindowDrawList()->AddRectFilled(
            viewportPos,
            { viewportPos.x + viewportSize.x, viewportPos.y + viewportSize.y },
            IM_COL32( 0, 0, 0, 255 )
        );

        ImGui::Image(
            m_SceneRenderer->GetOutputTexture()->GetImGuiHandle(),
            viewportSize
        );

        ImGui::End();
        ImGui::PopStyleVar(3);
    }
}
