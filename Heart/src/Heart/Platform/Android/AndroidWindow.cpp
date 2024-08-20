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

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_android.h"

namespace Heart
{
    static int HandleInputEvent(struct android_app* app, AInputEvent* inputEvent)
    {
        return ImGui_ImplAndroid_HandleInputEvent(inputEvent);
    }

    AndroidWindow::AndroidWindow(const WindowCreateInfo& createInfo)
        : Window(createInfo)
    {
        // Wait for native window to be available
        while (!AndroidApp::NativeWindow)
            PollEvents();

        HE_ENGINE_LOG_TRACE("Android native window = {0}", static_cast<void*>(AndroidApp::NativeWindow));

        AndroidApp::App->onInputEvent = HandleInputEvent;

        m_Window = AndroidApp::NativeWindow;
        s_WindowCount++;

        RecreateRenderContext();
    }

    AndroidWindow::~AndroidWindow()
    {
    
    }

    bool AndroidWindow::PollEvents()
    {
        bool recreated = false;

        if (AndroidApp::App->destroyRequested != 0)
        {
            App::Get().Close();
            return recreated;
        }

        if (AndroidApp::NativeWindow && AndroidApp::NativeWindow != m_Window)
        {
            // Window has been recreated, so we need to remake the render context
            
            HE_ENGINE_LOG_WARN("Detected android window recreation");

            m_Window = AndroidApp::NativeWindow;
            recreated = true;

            RecreateRenderContext();
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

        return recreated;
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

    void AndroidWindow::RecreateRenderContext()
    {
        m_RenderContext = nullptr;

        Flourish::RenderContextCreateInfo ctxCreateInfo;
        ctxCreateInfo.Window = (ANativeWindow*)m_Window;
        ctxCreateInfo.Width = m_WindowData.Width;
        ctxCreateInfo.Height = m_WindowData.Height;
        m_RenderContext = Flourish::RenderContext::Create(ctxCreateInfo);
    }
}

#endif
