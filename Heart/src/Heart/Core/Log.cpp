#include "hepch.h"
#include "Log.h"

#include "Heart/Container/HVector.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/details/fmt_helper.h"
#include "spdlog/details/os.h"

#if defined(HE_PLATFORM_MACOS) && defined(HE_DIST)
#include "Heart/Platform/MacOS/Utils.h"
#endif

namespace Heart
{
    template<typename Mutex>
    class LogListSink : public spdlog::sinks::base_sink<Mutex>
    {
    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            HE_PROFILE_FUNCTION();

            // Ignore trace because it fills up memory
            if (msg.level == spdlog::level::trace)
                return;

            spdlog::memory_buf_t formatted;
            auto localTime = spdlog::details::os::localtime(spdlog::log_clock::to_time_t(msg.time));
            spdlog::details::fmt_helper::pad2(localTime.tm_hour, formatted);
            formatted.push_back(':');
            spdlog::details::fmt_helper::pad2(localTime.tm_min, formatted);
            formatted.push_back(':');
            spdlog::details::fmt_helper::pad2(localTime.tm_sec, formatted);

            Logger::LockLogList();
            Logger::GetLogList().emplace_back(
                (LogLevel)msg.level,
                fmt::to_string(formatted),
                msg.logger_name,
                msg.payload
            );
            Logger::UnlockLogList();
        }

        void flush_() override 
        {
            
        }
    };

    void Logger::Initialize(const char* appName)
    {
        #if defined(HE_PLATFORM_MACOS) && defined(HE_DIST)
            std::string logPath = MacOS::Utils::GetApplicationSupportDirectory();
            logPath += "/";
            logPath += appName;
            logPath += "_Heart/";
            if (!std::filesystem::exists(logPath))
                std::filesystem::create_directory(logPath);
            logPath += appName;
            logPath += ".log";
        #else
            std::string logPath = appName;
            logPath += ".log";
        #endif
        
        HVector<spdlog::sink_ptr> logSinks = {
            CreateRef<spdlog::sinks::basic_file_sink_mt>(logPath, true),
        };
        
        #ifndef HE_DIST
        logSinks.Add(CreateRef<LogListSink<std::mutex>>());
        #endif

        logSinks[0]->set_pattern("[%T] [%l] %n: %v");

        s_EngineLogger = CreateRef<spdlog::logger>("ENGINE", logSinks.Begin(), logSinks.End());
        spdlog::register_logger(s_EngineLogger);
        #ifdef HE_DEBUG
            s_EngineLogger->set_level(spdlog::level::trace);
            s_EngineLogger->flush_on(spdlog::level::trace);
        #else
            s_EngineLogger->set_level(spdlog::level::info);
            s_EngineLogger->flush_on(spdlog::level::info);
        #endif

        s_ClientLogger = CreateRef<spdlog::logger>("CLIENT", logSinks.Begin(), logSinks.End());
        spdlog::register_logger(s_ClientLogger);
        #ifdef HE_DEBUG
            s_ClientLogger->set_level(spdlog::level::trace);
            s_ClientLogger->flush_on(spdlog::level::trace);
        #else
            s_ClientLogger->set_level(spdlog::level::info);
            s_ClientLogger->flush_on(spdlog::level::info);
        #endif
    }
}
