#pragma once

namespace Heart
{
    class Layer
    {
    public:
        Layer(const std::string& Name = "DefaultLayer");
        virtual ~Layer() = default;

        virtual void OnAttach() {}
        virtual void OnUpdate() {}
        virtual void OnImGuiRender() {}
        virtual void OnDetach() {}

    protected:
        std::string m_Name;
    };
}