#pragma once

#include "Heart/Scene/Scene.h"
#include "Heart/Core/Camera.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "imgui/imgui.h"

namespace HeartRuntime
{
    class Viewport
    {
    public:
        Viewport();

        void OnImGuiRender(Heart::Scene* sceneContext);

    private:
        Heart::Ref<Heart::SceneRenderer> m_SceneRenderer;
        Heart::SceneRenderSettings m_RenderSettings;
        Heart::Camera m_TestCamera = Heart::Camera(70.f, 0.1f, 500.f, 1.f);
        glm::vec3 m_TestCameraPos = { 0.f, 0.f, -1.f };
        glm::vec2 m_TestCameraRot = { 0.f, 0.f};
    };
}