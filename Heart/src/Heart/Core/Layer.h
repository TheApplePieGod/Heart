#pragma once

namespace Heart
{
    class Layer
    {
    public:
        Layer(const std::string& Name = "DefaultLayer");
        ~Layer() = default;

        virtual void OnAttach() {}
        virtual void OnUpdate() {}
        virtual void OnDetach() {}

    protected:
        std::string m_Name;
    };
}