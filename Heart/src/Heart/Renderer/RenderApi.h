#pragma once

#include "Heart/Renderer/GraphicsContext.h"

namespace Heart
{
    class RenderApi
    {
    public:
        enum class Type
        {
            None = 0, Vulkan = 1
        };

    public:
        virtual ~RenderApi() = default;

        virtual void SetViewport(GraphicsContext& context, u32 x, u32 y, u32 width, u32 height) = 0;

    private:
        
    };
}