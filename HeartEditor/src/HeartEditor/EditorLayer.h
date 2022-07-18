#pragma once

#include "HeartEditor/Toolbar.h"
#include "HeartEditor/MenuBar.h"
#include "Heart/Core/Layer.h"

namespace Heart
{
    class KeyPressedEvent;
    class MouseButtonPressedEvent;
    class MouseButtonReleasedEvent;
}

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
        Toolbar m_Toolbar;
        MenuBar m_MenuBar;
    };
}