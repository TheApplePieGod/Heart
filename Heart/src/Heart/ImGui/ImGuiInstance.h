#pragma once

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
        void OverrideImGuiConfig(const std::string& newBasePath);
        void ReloadImGuiConfig();
        
        void BeginFrame();
        void EndFrame();

    private:
        void Cleanup();
        void SetThemeColors();

    private:
        bool m_Initialized = false;
        std::string m_ImGuiConfigPath = "";
        Ref<Window> m_Window;
    };
}