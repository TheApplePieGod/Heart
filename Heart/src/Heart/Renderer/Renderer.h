#pragma once

#include "Heart/Renderer/RenderApi.h"

namespace Heart
{
    class Renderer
    {
    public:
        static void Initialize(RenderApi::Type apiType);
        static void Shutdown();

    public:
        static RenderApi& GetApi() { return *s_RenderApi; }
        static RenderApi::Type GetApiType() { return s_RenderApiType; }

    private:
        static Scope<RenderApi> s_RenderApi;
        static RenderApi::Type s_RenderApiType;
    };
}