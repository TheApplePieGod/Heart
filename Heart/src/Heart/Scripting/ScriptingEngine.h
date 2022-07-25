#pragma once

#include "Heart/Scripting/ManagedCallbacks.h"

namespace Heart
{
    class ScriptingEngine
    {
    public:
        static void Initialize();
        static void Shutdown();

    private:
        inline static ManagedCallbacks s_CoreCallbacks;
    };
}