#pragma once

#include "Heart/Renderer/RenderApi.h"

namespace Heart
{
    struct RenderStatistic
    {
        s64 Value;
    };

    class GraphicsContext;
    class Renderer
    {
    public:
        static void Initialize(RenderApi::Type apiType);
        static void Shutdown();

        static void OnWindowResize(GraphicsContext& context, u32 width, u32 height);

        inline static RenderApi& Api() { return *s_RenderApi; }
        inline static RenderApi::Type GetApiType() { return s_RenderApiType; }
        inline static bool IsUsingReverseDepth() { return s_UseReverseDepth; }
        inline static std::map<std::string, RenderStatistic>& GetStatistics() { return s_RenderStatistics; }
        inline static void PushStatistic(const std::string& name, s64 value) { s_RenderStatistics[name].Value += value; }
        inline static void ClearStatistic(const std::string& name) { s_RenderStatistics[name].Value = 0; }

        inline static constexpr u32 FrameBufferCount = 2;

    private:
        static Scope<RenderApi> s_RenderApi;
        static RenderApi::Type s_RenderApiType;
        static bool s_UseReverseDepth;
        static std::map<std::string, RenderStatistic> s_RenderStatistics;
    };
}