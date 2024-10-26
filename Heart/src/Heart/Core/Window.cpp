#include "hepch.h"
#include "Window.h"

#include "Heart/Events/WindowEvents.h"

#include "Flourish/Api/RenderContext.h"

#include "Heart/Platform/GLFWWindow.h"
#include "Heart/Platform/Android/AndroidWindow.h"

namespace Heart
{
    Ref<Window> Window::Create(const WindowCreateInfo& createInfo)
    {
        HE_ENGINE_LOG_INFO("Creating window ({0}x{1})", createInfo.Width, createInfo.Height);

        #ifdef HE_PLATFORM_ANDROID
            return CreateRef<AndroidWindow>(createInfo);
        #else
            return CreateRef<GLFWWindow>(createInfo);
        #endif
    }

    Window::Window(const WindowCreateInfo& createInfo)
    {
        m_WindowData.Title = createInfo.Title;
        m_WindowData.Width = createInfo.Width;
        m_WindowData.Height = createInfo.Height;
        m_WindowData.EmitEvent = HE_BIND_EVENT_FN(Window::EmitEvent);
    }

    Window::~Window()
    {
        s_WindowCount--;
    }

    void Window::EmitEvent(Event& event)
    {
        event.Map<WindowResizeEvent>(HE_BIND_EVENT_FN(Window::OnWindowResize));

        Emit(event);
    }

    bool Window::OnWindowResize(WindowResizeEvent& event)
    {
        m_RenderContext->UpdateDimensions(event.GetWidth(), event.GetHeight());
        
        return false;
    }
}
