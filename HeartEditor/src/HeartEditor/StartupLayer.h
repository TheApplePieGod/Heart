#pragma once

#include "HeartEditor/ProjectPicker.h"
#include "Heart/Core/Layer.h"

namespace Heart
{
    class KeyPressedEvent;
    class ButtonPressedEvent;
    class ButtonReleasedEvent;
}

namespace HeartEditor
{
    class StartupLayer : public Heart::Layer
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
        ProjectPicker m_ProjectPicker;
    };
}
