#pragma once

#include "Heart/Core/Window.h"

namespace Heart
{
    // For now, this class only utilizes the main window & graphics context
    class ImGuiInstance
    {
    public:
        ImGuiInstance(Ref<Window>& window);
        ~ImGuiInstance();

        void Recreate();
        
        void BeginFrame();
        void EndFrame();

    private:
        void Cleanup();
        void SetThemeColors();

    private:
        bool m_Initialized = false;
        Ref<Window> m_Window;
    };
}