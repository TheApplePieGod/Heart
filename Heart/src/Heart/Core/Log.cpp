#include "htpch.h"
#include "Log.h"

namespace Heart
{
    Ref<spdlog::logger> Logger::s_EngineLogger;
    Ref<spdlog::logger> Logger::s_ClientLogger;

    void Logger::Initialize()
    {
        auto console_sink = CreateRef<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("%^[%T] %n: %v%$");

        s_EngineLogger = CreateRef<spdlog::logger>("ENGINE", console_sink);
        spdlog::register_logger(s_EngineLogger);
        s_EngineLogger->set_level(spdlog::level::info);
        s_EngineLogger->flush_on(spdlog::level::info);

        s_ClientLogger = CreateRef<spdlog::logger>("CLIENT", console_sink);
        spdlog::register_logger(s_ClientLogger);
        s_ClientLogger->set_level(spdlog::level::trace);
        s_ClientLogger->flush_on(spdlog::level::trace);
    }
}