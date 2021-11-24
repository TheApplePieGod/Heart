#pragma once

#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Core/UUID.h"
#include "Heart/Core/Camera.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Renderer/EnvironmentMap.h"

namespace HeartEditor
{
namespace Widgets
{
    class MaterialEditor
    {
    public:
        MaterialEditor();
        
        void Initialize();
        void Shutdown();
        void OnImGuiRender(Heart::EnvironmentMap* envMap);

    private:
        void RenderSidebar();
        void RenderViewport(Heart::EnvironmentMap* envMap);

    private:
        Heart::UUID m_MaterialAsset = 0;
        Heart::Scope<Heart::SceneRenderer> m_SceneRenderer;
        Heart::Ref<Heart::Scene> m_Scene;
        Heart::Camera m_SceneCamera = Heart::Camera(70.f, 0.1f, 500.f, 1.f);
        glm::vec3 m_SceneCameraPosition = { 0.f, 0.f, 0.f };
        glm::vec2 m_SwivelRotation = { 0.f, 0.f };
        float m_Radius = 2.f;
        Heart::Entity m_DemoEntity;
        glm::vec2 m_WindowSizes = { 200.f, 5000.f };
    };
}
}