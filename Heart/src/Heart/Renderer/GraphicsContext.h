#pragma once

#include "Heart/Renderer/RenderApi.h"

namespace Heart
{
    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;

        static Scope<GraphicsContext> Create(RenderApi::Type apiType, void* window);

    private:

    };
}