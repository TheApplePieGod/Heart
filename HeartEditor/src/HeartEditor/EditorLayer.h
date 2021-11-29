#pragma once

#include "Heart/Core/Layer.h"
#include "Heart/Core/Timing.h"
#include "Heart/Core/UUID.h"
#include "HeartEditor/EditorCamera.h"
#include "Heart/Events/KeyboardEvents.h"
#include "Heart/Events/MouseEvents.h"
#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/EnvironmentMap.h"
#include "imgui/imgui.h"
#include "imguizmo/ImGuizmo.h"

#include "HeartEditor/MenuBar.h"

namespace HeartEditor
{
    class EditorLayer : public Heart::Layer
    {
    public:
        EditorLayer();
        ~EditorLayer() override;

        void OnAttach() override;
        void OnUpdate(Heart::Timestep ts) override;
        void OnImGuiRender() override;
        void OnDetach() override;

        void OnEvent(Heart::Event& event) override;

    protected:
        bool KeyPressedEvent(Heart::KeyPressedEvent& event);
        bool MouseButtonPressedEvent(Heart::MouseButtonPressedEvent& event);
        bool MouseButtonReleasedEvent(Heart::MouseButtonReleasedEvent& event);

    private:
        MenuBar m_MenuBar;
    };
}