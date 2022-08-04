#include "hepch.h"
#include "Viewport.h"

#include "HeartRuntime/RuntimeApp.h"
#include "Heart/Core/Camera.h"
#include "Heart/Core/Window.h"
#include "Heart/Input/Input.h"
#include "Heart/Renderer/Texture.h"

namespace HeartRuntime
{
    Viewport::Viewport()
    {
        m_SceneRenderer = Heart::CreateScope<Heart::SceneRenderer>();
        m_RenderSettings.DrawGrid = false;
    }

    void Viewport::OnImGuiRender(Heart::Scene* sceneContext)
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

#if 1
        f32 moveSpeed = 2.f; // m/s
        f32 mouseScale = 0.1f;
        auto ts = RuntimeApp::Get().GetLastTimestep();
        if (Heart::Input::IsKeyPressed(Heart::KeyCode::A))
            m_TestCameraPos -= (m_TestCamera.GetRightVector() * moveSpeed * static_cast<f32>(ts.StepSeconds()));
        if (Heart::Input::IsKeyPressed(Heart::KeyCode::D))
            m_TestCameraPos += (m_TestCamera.GetRightVector() * moveSpeed * static_cast<f32>(ts.StepSeconds()));
        if (Heart::Input::IsKeyPressed(Heart::KeyCode::W))
            m_TestCameraPos += (m_TestCamera.GetForwardVector() * moveSpeed * static_cast<f32>(ts.StepSeconds()));
        if (Heart::Input::IsKeyPressed(Heart::KeyCode::S))
            m_TestCameraPos -= (m_TestCamera.GetForwardVector() * moveSpeed * static_cast<f32>(ts.StepSeconds()));

        m_TestCameraRot.x += mouseScale * static_cast<f32>(Heart::Input::GetMouseDeltaX());
        m_TestCameraRot.y += -mouseScale * static_cast<f32>(Heart::Input::GetMouseDeltaY());
#endif

        float aspectRatio = viewportSize.x / viewportSize.y;
        m_TestCamera.UpdateAspectRatio(aspectRatio);
        m_TestCamera.UpdateViewMatrix(m_TestCameraRot.x, m_TestCameraRot.y, m_TestCameraPos);

        m_SceneRenderer->RenderScene(
            RuntimeApp::Get().GetWindow().GetContext(),
            sceneContext,
            m_TestCamera,
            m_TestCameraPos,
            m_RenderSettings
        );

        // draw the viewport background
        ImGui::GetWindowDrawList()->AddRectFilled(
            viewportPos,
            { viewportPos.x + viewportSize.x, viewportPos.y + viewportSize.y },
            IM_COL32( 255, 0, 0, 255 )
        );

        ImGui::Image(
            m_SceneRenderer->GetFinalTexture().GetImGuiHandle(),
            viewportSize
        );

        ImGui::End();
        ImGui::PopStyleVar(3);
    }
}