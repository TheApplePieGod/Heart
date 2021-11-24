#pragma once

#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Core/UUID.h"
#include "Heart/Core/Camera.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Renderer/EnvironmentMap.h"
#include "Heart/Renderer/Material.h"
#include "imgui/imgui.h"

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
        void OnImGuiRender(Heart::EnvironmentMap* envMap, Heart::UUID selectedMaterial, bool* dirty);

    private:
        void RenderSidebar(Heart::UUID selectedMaterial, bool* dirty);
        void RenderViewport(bool shouldRender);

    private:
        Heart::UUID m_MaterialAsset = 0;
        Heart::Scope<Heart::SceneRenderer> m_SceneRenderer;
        Heart::Ref<Heart::Scene> m_Scene;
        Heart::Camera m_SceneCamera = Heart::Camera(70.f, 0.1f, 500.f, 1.f);
        glm::vec3 m_SceneCameraPosition = { 0.f, 0.f, 0.f };
        glm::vec2 m_SwivelRotation = { 0.f, 0.f };
        float m_Radius = 2.f;
        Heart::Entity m_DemoEntity;
        glm::vec2 m_WindowSizes = { 0.f, 0.f };
        bool m_FirstRender = true;
        Heart::UUID m_LastMaterial = 0;
        ImGuiTextFilter m_TextureTextFilter;
        Heart::Material m_CachedMaterial;
    };
}
}