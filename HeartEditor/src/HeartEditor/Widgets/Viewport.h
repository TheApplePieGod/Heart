#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "imgui/imgui.h"
#include "imguizmo/ImGuizmo.h"
#include "glm/vec2.hpp"

namespace Heart
{
    class SceneRenderer;
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
        Viewport(const std::string& name, bool initialOpen);

        void OnImGuiRender() override;
        nlohmann::json Serialize() override;
        void Deserialize(const nlohmann::json& elem) override; 

        inline bool IsFocused() const { return m_ViewportInput; }
        inline bool IsHovered() const { return m_ViewportHover; }
        inline glm::vec2 GetRelativeMousePos() const { return m_ViewportMousePos; }
        inline glm::vec2 GetSize() const { return m_ViewportSize; }
        inline Heart::SceneRenderer& GetSceneRenderer() { return *m_SceneRenderer; }
        inline EditorCamera& GetCamera() { return *m_EditorCamera; }

        inline void SetFocused(bool focus) { m_ViewportInput = focus; }

    private:
        Heart::Ref<Heart::SceneRenderer> m_SceneRenderer;
        Heart::Ref<EditorCamera> m_EditorCamera;
        glm::vec2 m_ViewportMousePos; // mouse position relative to the viewport window
        glm::vec2 m_ViewportSize;
        bool m_ViewportInput = false;
        bool m_ViewportHover = false;
        ImGuizmo::MODE m_GizmoMode = ImGuizmo::MODE::LOCAL;
        ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
        int m_SelectedOutput = 0;
    };
}
}