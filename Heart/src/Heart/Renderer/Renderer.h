#pragma once

#include "Heart/Renderer/RenderApi.h"

namespace Heart
{
    class Renderer
    {
    public:
        static void Initialize(RenderApi::Type apiType);
        static void Shutdown();

    private:
        static RenderApi& GetApi() { return *s_RenderApi; }

    private:
        static Scope<RenderApi> s_RenderApi;
    };
}