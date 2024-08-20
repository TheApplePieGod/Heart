#pragma once

#ifdef HE_PLATFORM_ANDROID

#include "Heart/Core/Window.h"

namespace Heart
{
    class AndroidWindow : public Window
    {
    public:
        AndroidWindow(const WindowCreateInfo& createInfo);
        ~AndroidWindow();

        bool PollEvents() override;
        void EndFrame() override;

        void DisableCursor() override;
        void EnableCursor() override;

        void SetWindowTitle(const char* newTitle) override;
        void SetFullscreen(bool fullscreen) override;
        void ToggleFullscreen() override;
        bool IsFullscreen() const override;

    private:
        void RecreateRenderContext();
    };
}

#endif
