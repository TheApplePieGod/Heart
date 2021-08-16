#include "htpch.h"
#include "Log.h"

namespace Heart
{
    Ref<spdlog::logger> Logger::s_EngineLogger;

    void Logger::Initialize()
    {
        auto console_sink = CreateRef<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("%^[%T] %n: %v%$");

        s_EngineLogger = CreateRef<spdlog::logger>("ENGINE", console_sink);
        spdlog::register_logger(s_EngineLogger);
        s_EngineLogger->set_level(spdlog::level::trace);
        s_EngineLogger->flush_on(spdlog::level::trace);
    }
}