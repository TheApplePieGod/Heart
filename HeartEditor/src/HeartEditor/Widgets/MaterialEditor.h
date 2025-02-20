#pragma once

#include "HeartEditor/AssetPicker.h"
#include "HeartEditor/Widgets/Widget.h"
#include "Heart/Core/UUID.h"
#include "Heart/Core/Camera.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Scene/RenderScene.h"
#include "Heart/Renderer/Material.h"
#include "imgui/imgui.h"

namespace Heart
{
    class SceneRenderer;
}

namespace HeartEditor
{
namespace Widgets
{
    class MaterialEditor : public Widget
    {
    public:
        MaterialEditor(const Heart::HStringView8& name, bool initialOpen);
        ~MaterialEditor();
        
        void OnImGuiRenderPostSceneUpdate() override;
        
        void Reset();

        void SetSelectedMaterial(Heart::UUID material);
        void SetSelectedMaterial(const Heart::Material& material);

    private:
        void RenderSidebar();
        void RenderViewport(bool shouldRender);
        void RecomputeMaterial();

    private:
        Heart::UUID m_MaterialAsset = 0;
        Heart::UUID m_EditingMaterialAsset = 0;
        Heart::Scope<Heart::SceneRenderer> m_SceneRenderer;
        Heart::Ref<Heart::Scene> m_Scene;
        Heart::RenderScene m_RenderScene;
        Heart::Camera m_SceneCamera = Heart::Camera(70.f, 0.1f, 500.f, 1.f);
        glm::vec3 m_SceneCameraPosition = { 0.f, 0.f, 0.f };
        glm::vec3 m_SwivelRotation = { 0.f, 0.f, 0.f };
        float m_Radius = 2.f;
        Heart::Entity m_DemoEntity;
        glm::vec2 m_WindowSizes = { 300.f, 700.f };
        u32 m_RenderedFrames = 0;
        Heart::UUID m_LastMaterial = 0;
        Heart::UUID m_SelectedMaterial = 0;
        AssetPicker m_AssetPicker;
    };
}
}
