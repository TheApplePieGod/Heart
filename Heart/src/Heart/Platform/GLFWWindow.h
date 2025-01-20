#pragma once

#ifndef HE_PLATFORM_ANDROID

#include "Heart/Core/Window.h"

namespace Heart
{
    class GLFWWindow : public Window
    {
    public:
        GLFWWindow(const WindowCreateInfo& createInfo);
        ~GLFWWindow();

        bool PollEvents() override;
        void EndFrame() override;

        void DisableCursor() override;
        void EnableCursor() override;

        void SetWindowTitle(const char* newTitle) override;
        void SetFullscreen(bool fullscreen) override;
        void ToggleFullscreen() override;
        bool IsFullscreen() const override;
        float GetDPIScale() const override;

    private:
        struct GLFWWindowData
        {
            WindowData* DataPtr;
            u32 MousePositionSkips = 0;
        };

    private:
        GLFWWindowData m_GLFWWindowData;
    };
}

#endif
