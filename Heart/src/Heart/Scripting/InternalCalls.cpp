#include "hepch.h"
#include "InternalCalls.h"

#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"

namespace Heart
{
    void Log_Native(int level, MonoString* message)
    {
        Logger::GetClientLogger().log(spdlog::source_loc{}, static_cast<spdlog::level::level_enum>(level), mono_string_to_utf8(message));
    }

    void InternalCalls::Map()
    {
        /*
         * Heart.Core
         */
        mono_add_internal_call(
            "Heart.Core.Log::Log_Native(Heart.Core.Log/Level,string)",
            &Log_Native
        );
    }
}