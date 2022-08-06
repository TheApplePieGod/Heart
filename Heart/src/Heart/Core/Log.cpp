#include "hepch.h"
#include "Log.h"

#include "Heart/Container/HVector.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace Heart
{
    Ref<spdlog::logger> Logger::s_EngineLogger;
    Ref<spdlog::logger> Logger::s_ClientLogger;

    void Logger::Initialize()
    {
        HVector<spdlog::sink_ptr> logSinks = {
            CreateRef<spdlog::sinks::stdout_color_sink_mt>(),
            CreateRef<spdlog::sinks::basic_file_sink_mt>("Heart.log", true)
        };
        logSinks[0]->set_pattern("%^[%T] %n: %v%$");
        logSinks[1]->set_pattern("[%T] [%l] %n: %v");

        s_EngineLogger = CreateRef<spdlog::logger>("ENGINE", logSinks.Begin(), logSinks.End());
        spdlog::register_logger(s_EngineLogger);
        s_EngineLogger->set_level(spdlog::level::info);
        s_EngineLogger->flush_on(spdlog::level::info);

        s_ClientLogger = CreateRef<spdlog::logger>("CLIENT", logSinks.Begin(), logSinks.End());
        spdlog::register_logger(s_ClientLogger);
        s_ClientLogger->set_level(spdlog::level::trace);
        s_ClientLogger->flush_on(spdlog::level::trace);
    }
}