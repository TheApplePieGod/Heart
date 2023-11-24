#include "hepch.h"
#include "AndroidWindow.h"

#ifdef HE_PLATFORM_ANDROID

#include "Heart/Events/WindowEvents.h"
#include "Heart/Events/KeyboardEvents.h"
#include "Heart/Events/MouseEvents.h"
#include "Heart/Input/Input.h"

#include "Flourish/Api/RenderContext.h"

namespace Heart
{
    AndroidWindow::AndroidWindow(const WindowCreateInfo& createInfo)
        : Window(createInfo)
    {
        s_WindowCount++;
    }

    AndroidWindow::~AndroidWindow()
    {

    }

    void AndroidWindow::PollEvents()
    {

    }

    void AndroidWindow::EndFrame()
    {
        Flourish::Context::PushFrameRenderContext(m_RenderContext.get());
    }

    void AndroidWindow::DisableCursor()
    {

    }

    void AndroidWindow::EnableCursor()
    {

    }

    void AndroidWindow::SetWindowTitle(const char* newTitle)
    {
        m_WindowData.Title = newTitle;
    }

    void AndroidWindow::SetFullscreen(bool fullscreen)
    {

    }

    void AndroidWindow::ToggleFullscreen()
    {

    }

    bool AndroidWindow::IsFullscreen() const
    {
        return false;
    }
}

#endif
