#include "hepch.h"
#include "Window.h"

#include "Heart/Events/WindowEvents.h"
#include "Heart/Events/KeyboardEvents.h"
#include "Heart/Events/MouseEvents.h"
#include "Heart/Input/Input.h"
#include "GLFW/glfw3.h"

#include "Flourish/Api/RenderContext.h"

namespace Heart
{
    static void GLFWErrorCallback(int err, const char* desc)
    {
        HE_ENGINE_LOG_ERROR("GLFW Error ({0}): {1}", err, desc);
    }

    Ref<Window> Window::Create(const WindowCreateInfo& createInfo)
    {
        HE_ENGINE_LOG_INFO("Creating window ({0}x{1})", createInfo.Width, createInfo.Height);
        return CreateRef<Window>(createInfo);
    }

    Window::Window(const WindowCreateInfo& createInfo)
    {
        m_WindowData.Title = createInfo.Title;
        m_WindowData.Width = createInfo.Width;
        m_WindowData.Height = createInfo.Height;
        m_WindowData.EmitEvent = HE_BIND_EVENT_FN(Window::Emit);

        if (s_WindowCount == 0)
        {
            int success = glfwInit();
            HE_ENGINE_ASSERT(success, "Failed to initialize GLFW");
            glfwSetErrorCallback(GLFWErrorCallback);
        }

        if (Flourish::Context::BackendType() == Flourish::BackendType::Vulkan)
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            HE_ENGINE_ASSERT(glfwVulkanSupported(), "Device does not support vulkan rendering");
        }

        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_Window = glfwCreateWindow(createInfo.Width, createInfo.Height, createInfo.Title.Data(), nullptr, nullptr);
        s_WindowCount++;

        Flourish::RenderContextCreateInfo ctxCreateInfo;
        ctxCreateInfo.Window = m_Window;
        ctxCreateInfo.Width = createInfo.Width;
        ctxCreateInfo.Height = createInfo.Height;
        m_RenderContext = Flourish::RenderContext::Create(ctxCreateInfo);

        glfwSetWindowUserPointer(m_Window, &m_WindowData);
        
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EmitEvent(event);
		});

        glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            KeyCode keyCode = static_cast<KeyCode>(key);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(keyCode, false);
					data.EmitEvent(event);
				} break;
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(keyCode);
					data.EmitEvent(event);
				} break;
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(keyCode, true);
					data.EmitEvent(event);
				} break;
			}
		});

        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			WindowCloseEvent event;
			data.EmitEvent(event);
		});

        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
        {
            Input::UpdateMousePosition(xPos, yPos);
        });

        glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
        {
            Input::UpdateScrollOffset(xOffset, yOffset);
        });

        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            MouseCode mouseCode = static_cast<MouseCode>(button);

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(mouseCode);
					data.EmitEvent(event);
				} break;
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(mouseCode);
					data.EmitEvent(event);
				} break;
			}
        });

        //glfwSetCharCallback
    }

    Window::~Window()
    {
        glfwDestroyWindow(m_Window);
        s_WindowCount--;

        if (s_WindowCount == 0)
            glfwTerminate();
    }

    void Window::BeginFrame()
    {
        HE_PROFILE_FUNCTION();

    }

    void Window::PollEvents()
    {
        HE_PROFILE_FUNCTION();
        
        glfwPollEvents();
    }

    void Window::EndFrame()
    {
        Input::ClearDeltas();
        m_RenderContext->Present({ m_DependencyBuffers });
        m_DependencyBuffers.clear();
    }

    void Window::DisableCursor()
    {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void Window::EnableCursor()
    {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    void Window::SetWindowTitle(const char* newTitle)
    {
        m_WindowData.Title = newTitle;
        glfwSetWindowTitle(m_Window, newTitle);
    }

    void Window::SetFullscreen(bool fullscreen)
    {
        bool fullscreenStatus = IsFullscreen();
        if (fullscreenStatus == fullscreen)
            return;
        else
        {
            if (fullscreen)
            {
                // backup window position and window size
                glfwGetWindowPos(m_Window, &m_SavedWindowSizeAndPosition[2], &m_SavedWindowSizeAndPosition[3] );
                glfwGetWindowSize(m_Window, &m_SavedWindowSizeAndPosition[0], &m_SavedWindowSizeAndPosition[1] );

                // get resolution of monitor
                const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

                // switch to full screen
                glfwSetWindowMonitor(m_Window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
            }
            else
                glfwSetWindowMonitor(m_Window, nullptr,  m_SavedWindowSizeAndPosition[2], m_SavedWindowSizeAndPosition[3], m_SavedWindowSizeAndPosition[0], m_SavedWindowSizeAndPosition[1], 0 );
        }
    }

    void Window::ToggleFullscreen()
    {
        SetFullscreen(!IsFullscreen());
    }

    bool Window::IsFullscreen()
    {
        return glfwGetWindowMonitor(m_Window) != nullptr;
    }

    double Window::GetWindowTime()
    {
        return glfwGetTime() * 1000.0;
    }
}