#pragma once

#include "Heart/Container/HString.h"

namespace Heart
{
    class Window;

    // TODO: redo this class
    class ImGuiInstance
    {
    public:
        ImGuiInstance(Ref<Window>& window);
        ~ImGuiInstance();

        void Recreate();
        void OverrideImGuiConfig(const HString& newBasePath);
        void ReloadImGuiConfig();
        
        void BeginFrame();
        void EndFrame();

    private:
        void Cleanup();
        void SetThemeColors();

    private:
        bool m_Initialized = false;
        HString m_ImGuiConfigPath = "";
        Ref<Window> m_Window;
    };
}