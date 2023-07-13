#pragma once

#include "Heart/Scene/Scene.h"
#include "Heart/Core/Camera.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Scene/RenderScene.h"
#include "imgui/imgui.h"

namespace HeartRuntime
{
    class Viewport
    {
    public:
        Viewport();

        void Shutdown();

        void OnImGuiRender(
            Heart::RenderScene* renderScene,
            Heart::Scene* sceneContext,
            const Heart::SceneRenderSettings& settings
        );

    private:
        Heart::Ref<Heart::SceneRenderer> m_SceneRenderer;
        Heart::Camera m_Camera = Heart::Camera(70.f, 0.1f, 500.f, 1.f);
        glm::vec3 m_DebugCameraPos = { 0.f, 0.f, -1.f };
        glm::vec3 m_DebugCameraRot = { 0.f, 0.f, 0.f };
    };
}
