#include "htpch.h"
#include "Window.h"

#include "Heart/Events/WindowEvents.h"

namespace Heart
{
    int Window::s_WindowCount = 0;

    static void GLFWErrorCallback(int err, const char* desc)
    {
        HT_ENGINE_LOG_ERROR("GLFW Error ({0}): {1}", err, desc);
    }

    Scope<Window> Window::Create(const WindowSettings& settings)
    {
        HT_ENGINE_LOG_INFO("Creating window ({0}x{1})", settings.Width, settings.Height);
        return CreateScope<Window>(settings);
    }

    Window::Window(const WindowSettings& settings)
    {
        m_WindowData.Title = settings.Title;
        m_WindowData.Width = settings.Width;
        m_WindowData.Height = settings.Height;
        m_WindowData.EmitEvent = HT_BIND_EVENT_FN(Window::Emit);

        if (s_WindowCount == 0)
        {
            int success = glfwInit();
            HT_ENGINE_ASSERT(success, "Failed to initialize GLFW");
        }

        m_Window = glfwCreateWindow(settings.Width, settings.Height, settings.Title.c_str(), nullptr, nullptr);
        s_WindowCount++;

        glfwSetWindowUserPointer(m_Window, &m_WindowData);

        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EmitEvent(event);
		});

        //glfwSetWindowCloseCallback
        //glfwSetKeyCallback
        //glfwSetCharCallback
        //glfwSetMouseButtonCallback
        //glfwSetScrollCallback
        //glfwSetCursorPosCallback
    }

    Window::~Window()
    {
        glfwDestroyWindow(m_Window);
        s_WindowCount--;

        if (s_WindowCount == 0)
            glfwTerminate();
    }

    void Window::OnUpdate()
    {
        glfwPollEvents();
    }
}