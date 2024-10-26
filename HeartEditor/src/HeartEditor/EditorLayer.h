#pragma once

#include "HeartEditor/Toolbar.h"
#include "HeartEditor/MenuBar.h"
#include "Heart/Core/Layer.h"

namespace Heart
{
    class KeyPressedEvent;
    class ButtonPressedEvent;
    class ButtonReleasedEvent;
}

namespace HeartEditor
{
    class EditorLayer : public Heart::Layer
    {
    public:
        void OnAttach() override;
        void OnUpdate(Heart::Timestep ts) override;
        void OnDetach() override;

        void OnEvent(Heart::Event& event) override;

    protected:
        bool KeyPressedEvent(Heart::KeyPressedEvent& event);
        bool ButtonPressedEvent(Heart::ButtonPressedEvent& event);
        bool ButtonReleasedEvent(Heart::ButtonReleasedEvent& event);

    private:
        Toolbar m_Toolbar;
        MenuBar m_MenuBar;
    };
}
