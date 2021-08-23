#pragma once

#include "Heart/Events/EventEmitter.h"
#include "Heart/Core/Timestep.h"

namespace Heart
{
    class Layer : public EventListener
    {
    public:
        Layer(const std::string& Name = "DefaultLayer");
        virtual ~Layer() = default;

        virtual void OnAttach() {}
        virtual void OnUpdate(Timestep ts) {}
        virtual void OnImGuiRender() {}
        virtual void OnDetach() {}

        virtual void OnEvent(Event& event) {};

    protected:
        std::string m_Name;
    };
}