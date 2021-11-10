#pragma once

#include "Heart/Core/Layer.h"
#include "Heart/Core/Timing.h"
#include "HeartEditor/EditorCamera.h"
#include "Heart/Events/KeyboardEvents.h"
#include "Heart/Events/MouseEvents.h"
#include "Heart/Scene/Scene.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "imgui/imgui.h"
#include "imguizmo/ImGuizmo.h"

#include "HeartEditor/Widgets/MenuBar.h"
#include "HeartEditor/Widgets/SceneHierarchyPanel.h"
#include "HeartEditor/Widgets/PropertiesPanel.h"

namespace HeartEditor
{
    struct EditorWidgets
    {
        Widgets::MenuBar MainMenuBar;
        Widgets::SceneHierarchyPanel SceneHierarchyPanel;
        Widgets::PropertiesPanel PropertiesPanel;
    };

    class EditorLayer : public Heart::Layer
    {
    public:
        EditorLayer();
        ~EditorLayer() override;

        void OnAttach() override;
        void OnUpdate(Heart::Timestep ts) override;
        void OnImGuiRender() override;
        void OnDetach() override;

        void RenderViewport();

        void OnEvent(Heart::Event& event) override;

    protected:
        bool KeyPressedEvent(Heart::KeyPressedEvent& event);
        bool MouseButtonPressedEvent(Heart::MouseButtonPressedEvent& event);
        bool MouseButtonReleasedEvent(Heart::MouseButtonReleasedEvent& event);

    private:
        EditorWidgets m_Widgets;
        Heart::Ref<Heart::Scene> m_ActiveScene;
        Heart::Scope<Heart::SceneRenderer> m_SceneRenderer;
        Heart::Scope<EditorCamera> m_EditorCamera;
        glm::vec2 m_ViewportMousePos; // mouse position relative to the viewport window
        glm::vec2 m_ViewportSize;
        bool m_ViewportInput = false;
        bool m_ViewportHover = false;
        ImGuizmo::MODE m_GizmoMode = ImGuizmo::MODE::LOCAL;
        ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
        Heart::Scope<Heart::TextureRegistry> m_EditorTextures;
    };
}