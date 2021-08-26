#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace Heart
{
    class Logger
    {
    public:
        static void Initialize();

        inline static spdlog::logger& GetEngineLogger() { return *s_EngineLogger; }
        inline static spdlog::logger& GetClientLogger() { return *s_ClientLogger; }

    private:
        static Ref<spdlog::logger> s_EngineLogger;
        static Ref<spdlog::logger> s_ClientLogger;
    };
}

#define HE_ENGINE_LOG_TRACE(...) ::Heart::Logger::GetEngineLogger().trace(__VA_ARGS__)
#define HE_ENGINE_LOG_INFO(...) ::Heart::Logger::GetEngineLogger().info(__VA_ARGS__)
#define HE_ENGINE_LOG_WARN(...) ::Heart::Logger::GetEngineLogger().warn(__VA_ARGS__)
#define HE_ENGINE_LOG_ERROR(...) ::Heart::Logger::GetEngineLogger().error(__VA_ARGS__)
#define HE_ENGINE_LOG_CRITICAL(...) ::Heart::Logger::GetEngineLogger().critical(__VA_ARGS__)

#define HE_CLIENT_LOG_TRACE(...) ::Heart::Logger::GetClientLogger().trace(__VA_ARGS__)
#define HE_CLIENT_LOG_INFO(...) ::Heart::Logger::GetClientLogger().info(__VA_ARGS__)
#define HE_CLIENT_LOG_WARN(...) ::Heart::Logger::GetClientLogger().warn(__VA_ARGS__)
#define HE_CLIENT_LOG_ERROR(...) ::Heart::Logger::GetClientLogger().error(__VA_ARGS__)
#define HE_CLIENT_LOG_CRITICAL(...) ::Heart::Logger::GetClientLogger().critical(__VA_ARGS__)