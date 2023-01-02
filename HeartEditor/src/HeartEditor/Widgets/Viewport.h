#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "imgui/imgui.h"
#include "imguizmo/ImGuizmo.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

namespace Heart
{
    class SceneRenderer;
    class Camera;
}

namespace HeartEditor
{
    class EditorCamera;
}

namespace HeartEditor
{
namespace Widgets
{
    class Viewport : public Widget
    {
    public:
        Viewport(const Heart::HStringView8& name, bool initialOpen);

        void OnImGuiRender() override;
        nlohmann::json Serialize() override;
        void Deserialize(const nlohmann::json& elem) override;

        void UpdateCamera();
        void SetFocused(bool focus);
        void ResetEditorCamera();
        
        inline bool IsFocused() const { return m_ViewportInput; }
        inline bool IsHovered() const { return m_ViewportHover; }
        inline bool ShouldCameraAttach() const { return m_AttachCamera; }
        inline glm::vec2 GetRelativeMousePos() const { return m_ViewportMousePos; }
        inline glm::vec2 GetSize() const { return m_ViewportSize; }
        inline Heart::SceneRenderer& GetSceneRenderer() { return *m_SceneRenderer; }
        inline Heart::Camera& GetActiveCamera() { return *m_ActiveCamera; }
        inline glm::vec3 GetActiveCameraPosition() const { return m_ActiveCameraPos; }
        inline glm::vec3 GetActiveCameraRotation() const { return m_ActiveCameraRot; }
        inline EditorCamera& GetEditorCamera() { return *m_EditorCamera; }

    private:
        Heart::Ref<Heart::SceneRenderer> m_SceneRenderer;
        Heart::Ref<Heart::Camera> m_ActiveCamera;
        Heart::Ref<EditorCamera> m_EditorCamera;
        glm::vec3 m_ActiveCameraPos;
        glm::vec3 m_ActiveCameraRot;
        glm::vec3 m_StoredCameraPos;
        glm::vec3 m_StoredCameraRot;
        bool m_AttachCamera = true;
        glm::vec2 m_ViewportMousePos; // mouse position relative to the viewport window
        glm::vec2 m_ViewportSize;
        bool m_ViewportInput = false;
        bool m_ViewportHover = false;
        f32 m_AspectRatio = 1.f;
        ImGuizmo::MODE m_GizmoMode = ImGuizmo::MODE::LOCAL;
        ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
        int m_SelectedOutput = 0;
        int m_SelectedOutputMip = 0;
    };
}
}
