#pragma once

#include "Heart/Core/Layer.h"

namespace HeartEditor
{
    class EditorLayer : public Heart::Layer
    {
    public:
        void OnAttach() override;
        void OnUpdate() override;
        void OnImGuiRender() override;
        void OnDetach() override;

    private:

    };
}