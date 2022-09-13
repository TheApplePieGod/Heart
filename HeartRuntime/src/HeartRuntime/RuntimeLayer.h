#pragma once

#include "HeartRuntime/Viewport.h"
#include "Heart/Core/Layer.h"
#include "Heart/Scene/Scene.h"
#include "Heart/Events/KeyboardEvents.h"

namespace HeartRuntime
{
    class RuntimeLayer : public Heart::Layer
    {
    public:
        RuntimeLayer();
        ~RuntimeLayer() override;

        void OnAttach() override;
        void OnUpdate(Heart::Timestep ts) override;
        void OnImGuiRender() override;
        void OnDetach() override;

        void OnEvent(Heart::Event& event) override;

    private:
        void LoadProject();
        bool KeyPressedEvent(Heart::KeyPressedEvent& event);

    private:
        Heart::Ref<Heart::Scene> m_RuntimeScene;
        Viewport m_Viewport;
    };
}