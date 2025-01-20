#pragma once

#include "Heart/Container/HString8.h"

struct ImFont;

namespace Heart
{
    class Window;
    
    // TODO: redo this class
    class ImGuiInstance
    {
    public:
        ImGuiInstance(Ref<Window>& window);
        ~ImGuiInstance();

        void UpdateWindow(Ref<Window>& window);
        void Recreate();
        void OverrideImGuiConfig(const HStringView8& newBasePath);
        void ReloadImGuiConfig();
        
        void BeginFrame();
        void EndFrame();

        inline ImFont* GetSmallFont() const { return m_SmallFont; };
        inline ImFont* GetMediumFont() const { return m_MediumFont; };
        inline ImFont* GetLargeFont() const { return m_LargeFont; };

    private:
        void Cleanup(bool willRecreate);
        void EndFrameInternal(bool render);
        void UploadFonts();
        void SetThemeColors();

    private:
        bool m_InFrame = false;
        bool m_Initialized = false;
        HString8 m_ImGuiConfigPath = "";
        Ref<Window> m_Window;

        ImFont* m_SmallFont = nullptr;
        ImFont* m_MediumFont = nullptr;
        ImFont* m_LargeFont = nullptr;
        
        // Used for vulkan imgui backend
        void* m_DescriptorPool = nullptr;
        void* m_DescriptorSetLayout = nullptr;
    };
}
