#pragma once

#include "Heart/Renderer/RenderApi.h"

namespace Heart
{
    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;

        virtual void InitializeImGui() = 0;
        virtual void ShutdownImGui() = 0;
        virtual void ImGuiBeginFrame() = 0;
        virtual void ImGuiEndFrame() = 0;

    public:
        static Scope<GraphicsContext> Create(void* window);

    private:

    };
}