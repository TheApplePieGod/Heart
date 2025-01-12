#include "hepch.h"
#include "Log.h"

#include "Heart/Container/HVector.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/details/fmt_helper.h"
#include "spdlog/details/os.h"

#if defined(HE_PLATFORM_MACOS)
#include "Heart/Platform/MacOS/Utils.h"
#endif

#ifdef HE_PLATFORM_ANDROID
#include <android/log.h>
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
                Logger::GetNextLogId(),
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

#ifdef HE_PLATFORM_ANDROID
    template<typename Mutex>
    class AndroidLogSink : public spdlog::sinks::base_sink<Mutex>
    {
    public:
        AndroidLogSink(const char* appName)
            : m_AppName(appName)
        {}

    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            spdlog::memory_buf_t formatted;
            auto localTime = spdlog::details::os::localtime(spdlog::log_clock::to_time_t(msg.time));
            spdlog::details::fmt_helper::pad2(localTime.tm_hour, formatted);
            formatted.push_back(':');
            spdlog::details::fmt_helper::pad2(localTime.tm_min, formatted);
            formatted.push_back(':');
            spdlog::details::fmt_helper::pad2(localTime.tm_sec, formatted);

            int androidLogLevel;
            switch ((LogLevel)msg.level)
            {
                case LogLevel::Trace: { androidLogLevel = ANDROID_LOG_VERBOSE; } break;
                case LogLevel::Debug: { androidLogLevel = ANDROID_LOG_DEBUG; } break;
                case LogLevel::Info: { androidLogLevel = ANDROID_LOG_INFO; } break;
                case LogLevel::Warn: { androidLogLevel = ANDROID_LOG_WARN; } break;
                case LogLevel::Error: { androidLogLevel = ANDROID_LOG_ERROR; } break;
                case LogLevel::Critical: { androidLogLevel = ANDROID_LOG_ERROR; } break;
            }

            auto timestamp =  fmt::to_string(formatted);
            ((void)__android_log_print(
                androidLogLevel,
                m_AppName.c_str(),
                "[%s] [%s] %s: %s",
                timestamp.data(),
                Heart::LogListEntry::TypeStrings[msg.level],
                msg.logger_name.data(),
                msg.payload.data()
            ));
        }

        void flush_() override 
        {
            
        }

    private:
        std::string m_AppName;
    };
#endif

    void Logger::Initialize(const char* appName)
    {
        std::string logPath;
        #if defined(HE_PLATFORM_MACOS)
            bool isPackaged = MacOS::Utils::IsAppPackaged();
            if (isPackaged)
            {
                // If the app is bundled, we cannot write logs there, so put
                // them somewhere else.
                logPath = MacOS::Utils::GetApplicationSupportDirectory();
                logPath += "/";
                logPath += appName;
                logPath += "_Heart/";
                if (!std::filesystem::exists(logPath))
                    std::filesystem::create_directory(logPath);
            }

            logPath += appName;
            logPath += ".log";
        #else
            logPath = appName;
            logPath += ".log";
        #endif
        
        HVector<spdlog::sink_ptr> logSinks = {
            #ifdef HE_PLATFORM_ANDROID
            CreateRef<AndroidLogSink<std::mutex>>(appName),
            #else
            CreateRef<spdlog::sinks::basic_file_sink_mt>(logPath, true),
            #endif
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

        HE_ENGINE_LOG_INFO("Logger initialized");
        HE_ENGINE_LOG_INFO("Log Directory: '{}'", logPath);
    }
}
