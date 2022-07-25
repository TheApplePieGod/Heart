#pragma once

#include "Heart/Scripting/ManagedCallbacks.h"

namespace Heart
{
    class ScriptingEngine
    {
    public:
        static void Initialize();
        static void Shutdown();

        static bool LoadClientPlugin(const std::string& absolutePath);
        static bool UnloadClientPlugin();

    private:
        inline static ManagedCallbacks s_CoreCallbacks;
        inline static bool s_ClientPluginLoaded;
    };
}