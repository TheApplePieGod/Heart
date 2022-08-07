#pragma once

#include "spdlog/spdlog.h"

namespace Heart
{
    enum class LogLevel
    {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Critical = 5
    };

    struct LogListEntry
    {
        LogListEntry(
            LogLevel level,
            spdlog::string_view_t timestamp,
            spdlog::string_view_t source,
            spdlog::string_view_t message)
            : Level(level),
              Timestamp(timestamp.begin(), timestamp.size()),
              Source(source.begin(), source.size()),
              Message(message.begin(), message.size())
        {}

        inline static const char* TypeStrings[] = {
            "Trace", "Debug", "Info", "Warn", "Error", "Critical"
        };

        LogLevel Level;
        std::string Timestamp;
        std::string Source;
        std::string Message;
    };

    class Logger
    {
    public:
        static void Initialize();

        inline static spdlog::logger& GetEngineLogger() { return *s_EngineLogger; }
        inline static spdlog::logger& GetClientLogger() { return *s_ClientLogger; }
        inline static auto& GetLogList() { return s_LogList; }
        inline static void LockLogList() { s_LogListLock.lock(); }
        inline static void UnlockLogList() { s_LogListLock.unlock(); }

    private:
        inline static Ref<spdlog::logger> s_EngineLogger;
        inline static Ref<spdlog::logger> s_ClientLogger;
        inline static std::vector<LogListEntry> s_LogList;
        inline static std::mutex s_LogListLock;
    };
}

#define HE_ENGINE_LOG_TRACE(...) ::Heart::Logger::GetEngineLogger().trace(__VA_ARGS__)
#define HE_ENGINE_LOG_DEBUG(...) ::Heart::Logger::GetEngineLogger().debug(__VA_ARGS__)
#define HE_ENGINE_LOG_INFO(...) ::Heart::Logger::GetEngineLogger().info(__VA_ARGS__)
#define HE_ENGINE_LOG_WARN(...) ::Heart::Logger::GetEngineLogger().warn(__VA_ARGS__)
#define HE_ENGINE_LOG_ERROR(...) ::Heart::Logger::GetEngineLogger().error(__VA_ARGS__)
#define HE_ENGINE_LOG_CRITICAL(...) ::Heart::Logger::GetEngineLogger().critical(__VA_ARGS__)

#define HE_LOG_TRACE(...) ::Heart::Logger::GetClientLogger().trace(__VA_ARGS__)
#define HE_LOG_DEBUG(...) ::Heart::Logger::GetClientLogger().debug(__VA_ARGS__)
#define HE_LOG_INFO(...) ::Heart::Logger::GetClientLogger().info(__VA_ARGS__)
#define HE_LOG_WARN(...) ::Heart::Logger::GetClientLogger().warn(__VA_ARGS__)
#define HE_LOG_ERROR(...) ::Heart::Logger::GetClientLogger().error(__VA_ARGS__)
#define HE_LOG_CRITICAL(...) ::Heart::Logger::GetClientLogger().critical(__VA_ARGS__)