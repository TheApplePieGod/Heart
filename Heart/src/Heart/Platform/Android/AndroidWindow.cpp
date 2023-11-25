#include "hepch.h"
#include "AndroidWindow.h"

#ifdef HE_PLATFORM_ANDROID

#include "Heart/Events/WindowEvents.h"
#include "Heart/Events/KeyboardEvents.h"
#include "Heart/Events/MouseEvents.h"
#include "Heart/Input/Input.h"
#include "Heart/Core/App.h"
#include "Heart/Platform/Android/AndroidApp.h"

#include "Flourish/Api/RenderContext.h"

namespace Heart
{
    AndroidWindow::AndroidWindow(const WindowCreateInfo& createInfo)
        : Window(createInfo)
    {
        // Wait for native window to be available
        while (!AndroidApp::NativeWindow)
            PollEvents();

        HE_ENGINE_LOG_TRACE("Android native window = {0}", static_cast<void*>(AndroidApp::NativeWindow));

        Flourish::RenderContextCreateInfo ctxCreateInfo;
        ctxCreateInfo.Window = AndroidApp::NativeWindow;
        ctxCreateInfo.Width = createInfo.Width;
        ctxCreateInfo.Height = createInfo.Height;
        m_RenderContext = Flourish::RenderContext::Create(ctxCreateInfo);

        m_Window = AndroidApp::NativeWindow;
        s_WindowCount++;
    }

    AndroidWindow::~AndroidWindow()
    {
    
    }

    void AndroidWindow::PollEvents()
    {
        if (AndroidApp::App->destroyRequested != 0)
        {
            App::Get().Close();
            return;
        }

        while (true)
        {
            // Poll and process the Android OS system events.
            struct android_poll_source *source = nullptr;
            int events = 0;
            int timeoutMilliseconds = AndroidApp::Paused ? -1 : 0;
            if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, (void **)&source) >= 0)
            {
                if (source != nullptr)
                    source->process(AndroidApp::App, source);

                continue;
            }

            break;
        }
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
