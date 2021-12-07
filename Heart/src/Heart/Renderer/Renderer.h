#pragma once

#include "Heart/Renderer/RenderApi.h"

namespace Heart
{
    class GraphicsContext;
    class Renderer
    {
    public:
        static void Initialize(RenderApi::Type apiType);
        static void Shutdown();

        static void OnWindowResize(GraphicsContext& context, u32 width, u32 height);

        static RenderApi& Api() { return *s_RenderApi; }
        static RenderApi::Type GetApiType() { return s_RenderApiType; }
        static bool IsUsingReverseDepth() { return s_UseReverseDepth; }

    private:
        static Scope<RenderApi> s_RenderApi;
        static RenderApi::Type s_RenderApiType;
        static bool s_UseReverseDepth;
    };
}