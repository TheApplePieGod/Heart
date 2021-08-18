#pragma once

#include "Heart/Core/Window.h"

namespace Heart
{
    // For now, this class only utilizes the main window & graphics context
    class ImGuiInstance
    {
    public:
        void Initialize();
        void Shutdown();

        void Recreate();
        
        void BeginFrame();
        void EndFrame();

    private:
        void Cleanup();

    private:
        bool m_Initialized = false;

    };
}