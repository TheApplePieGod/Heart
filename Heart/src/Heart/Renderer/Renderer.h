#pragma once

#include "Heart/Renderer/RenderApi.h"

namespace Heart
{
    struct RenderStatistic
    {
        u32 Value;
    };

    class GraphicsContext;
    class Renderer
    {
    public:
        static void Initialize(RenderApi::Type apiType);
        static void Shutdown();
        static void ClearStatistics();

        static void OnWindowResize(GraphicsContext& context, u32 width, u32 height);

        inline static RenderApi& Api() { return *s_RenderApi; }
        inline static RenderApi::Type GetApiType() { return s_RenderApiType; }
        inline static bool IsUsingReverseDepth() { return s_UseReverseDepth; }
        inline static std::map<std::string, RenderStatistic>& GetStatistics() { return s_RenderStatisticsLastFrame; }
        inline static void PushStatistic(const std::string& name, u32 value) { s_RenderStatistics[name].Value += value; }

    private:
        static Scope<RenderApi> s_RenderApi;
        static RenderApi::Type s_RenderApiType;
        static bool s_UseReverseDepth;
        static std::map<std::string, RenderStatistic> s_RenderStatistics;
        static std::map<std::string, RenderStatistic> s_RenderStatisticsLastFrame;
    };
}